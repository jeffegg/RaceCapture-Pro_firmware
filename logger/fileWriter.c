/*
 * fileWriter.c
 *
 *  Created on: Feb 29, 2012
 *      Author: brent
 */
#include "fileWriter.h"
#include "task.h"
#include "semphr.h"
#include "modp_numtoa.h"
#include "sdcard.h"
#include "sampleRecord.h"
#include "loggerHardware.h"
#include "usb_comm.h"
#include "string.h"

static int g_writingActive;
static FIL g_logfile;
static xQueueHandle g_sampleRecordQueue = NULL;

#define FILE_WRITER_TASK_PRIORITY				( tskIDLE_PRIORITY + 4 )
#define FILE_WRITER_STACK_SIZE  				200
#define SAMPLE_RECORD_QUEUE_SIZE				10
#define MAX_LOG_FILE_INDEX 						99999

//wait time for sample queue. can be portMAX_DELAY to wait forever, or zero to not wait at all
//#define SAMPLE_QUEUE_WAIT_TIME					0
#define SAMPLE_QUEUE_WAIT_TIME					portMAX_DELAY

portBASE_TYPE queueLogfileRecord(SampleRecord * sr){
	if (NULL != g_sampleRecordQueue){
		return xQueueSend(g_sampleRecordQueue, &sr, SAMPLE_QUEUE_WAIT_TIME);
	}
	else{
		return errQUEUE_EMPTY;
	}
}

static void fileWriteString(FIL *f, char *s){
	f_puts(s,f);
	f_sync(f);
}

static void fileWriteQuotedString(FIL *f, char *s){
	fileWriteString(f,"\"");
	fileWriteString(f,s);
	fileWriteString(f,"\"");
}


static void fileWriteInt(FIL *f, int num){
	char buf[10];
	modp_itoa10(num,buf);
	fileWriteString(f,buf);
}

static void fileWriteFloat(FIL *f, float num, int precision){
	char buf[20];
	modp_ftoa(num, buf, precision);
	fileWriteString(f,buf);
}

static void fileWriteDouble(FIL *f, double num, int precision){
	char buf[30];
	modp_dtoa(num, buf, precision);
	fileWriteString(f,buf);
}

static void writeHeaders(FIL *f, SampleRecord *sr){

	int headerCount = 0;
	for (int i = 0; i < SAMPLE_RECORD_CHANNELS;i++){
		ChannelConfig *cfg = (sr->Samples[i].channelConfig);
		if (SAMPLE_DISABLED != cfg->sampleRate){
			if (headerCount > 0) fileWriteString(f,",");
			//SendUint((unsigned int)&(cfg));
			fileWriteQuotedString(f,cfg->label);
			headerCount++;
		}
	}
	fileWriteString(f,"\r\n");
}


static void writeSampleRecord(FIL * logfile, SampleRecord * sampleRecord){

	if (NULL == sampleRecord) return;

	for (int i = 0; i < SAMPLE_RECORD_CHANNELS; i++){
		ChannelSample *sample = &(sampleRecord->Samples[i]);
		ChannelConfig * channelConfig = sample->channelConfig;

		if (SAMPLE_DISABLED == channelConfig->sampleRate) continue;

		if (i > 0) fileWriteString(logfile,",");

		if (sample->intValue == NIL_SAMPLE) continue;

		int precision = sample->channelConfig->precision;
		if (precision > 0){
			fileWriteFloat(logfile,sample->floatValue,precision);
		}
		else{
			fileWriteInt(logfile, sample->intValue);
		}
	}
	fileWriteString(logfile,"\n");
}

static int openNextLogFile(FIL *f){

	char filename[13];
	int i = 0;
	int rc;
	for (; i < MAX_LOG_FILE_INDEX; i++){
		strcpy(filename,"rc_");
		char numBuf[12];
		modp_itoa10(i,numBuf);
		strcat(filename,numBuf);
		strcat(filename,".log");
		rc = f_open(f,filename, FA_WRITE | FA_CREATE_NEW);
		if ( rc == 0 ) break;
		f_close(f);
	}
	if (i >= MAX_LOG_FILE_INDEX) return -2;
	return rc;
}

void fileWriterTask(void *params){

	SampleRecord *sr = NULL;
	while(1){
		//wait for the next sample record
		xQueueReceive(g_sampleRecordQueue, &(sr), portMAX_DELAY);

		ToggleLED(LED3);
		if (NULL != sr && 0 == g_writingActive){
			//start of a new logfile
			int rc = InitFS();
			if (0 != rc) continue;

			//open next log file
			rc = openNextLogFile(&g_logfile);
			if (0 != rc) continue;

			g_writingActive = 1;
			writeHeaders(&g_logfile,sr);
		}

		if (g_writingActive){
			//a null sample record means end of sample run; like an EOF
			if (NULL != sr){
				writeSampleRecord(&g_logfile,sr);
			}
			else{
				f_close(&g_logfile);
				g_writingActive = 0;
			}
		}
	}
}

void createFileWriterTask(){

	g_writingActive = 0;
	g_sampleRecordQueue = xQueueCreate(SAMPLE_RECORD_QUEUE_SIZE,sizeof( SampleRecord *));
	if (NULL == g_sampleRecordQueue){
		//TODO log error
		return;
	}
	xTaskCreate( fileWriterTask,( signed portCHAR * ) "fileWriter", FILE_WRITER_STACK_SIZE, NULL, FILE_WRITER_TASK_PRIORITY, NULL );
}