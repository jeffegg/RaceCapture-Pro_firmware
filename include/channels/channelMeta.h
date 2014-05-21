#ifndef CHANNELMETA_H_
#define CHANNELMETA_H_
#include "stddef.h"

#define MAX_CHANNEL_COUNT			90
#define DEFAULT_LABEL_LENGTH		11
#define DEFAULT_UNITS_LENGTH		11

#define CHANNEL_SYSTEM_CHANNEL_FLAG 	0

#define CHANNEL_TYPE_UNKNOWN			0
#define CHANNEL_TYPE_ANALOG				1
#define CHANNEL_TYPE_FREQ				2
#define CHANNEL_TYPE_GPIO				3
#define CHANNEL_TYPE_PWM				4
#define CHANNEL_TYPE_IMU				5
#define CHANNEL_TYPE_GPS				6
#define CHANNEL_TYPE_STATISTICS			7


typedef struct _ChannelName{
	char label[DEFAULT_LABEL_LENGTH];
	char units[DEFAULT_UNITS_LENGTH];
	unsigned char precision;
	unsigned char flags;
	float min;
	float max;
} Channel;

typedef struct _Channels{
	size_t count;
	Channel channels[MAX_CHANNEL_COUNT];
} Channels;

int flash_default_channels(void);
const Channel * get_channel(size_t id);
const Channels * get_channels();
size_t filter_channel_id(size_t id);
size_t find_channel_id(const char * name);
int is_channel_type(const Channel *channel, unsigned char type);
void set_channel_type(Channel *channel, unsigned char type);
int is_system_channel(const Channel *channel);
int flash_channels(const Channels *source, size_t rawSize);
int add_channel(const Channel *channel, int mode, size_t index);
#define DEFAULT_GPS_POSITION_PRECISION 		6
#define DEFAULT_GPS_RADIUS_PRECISION 		5
#define DEFAULT_VOLTAGE_SCALING_PRECISION	2
#define DEFAULT_ANALOG_SCALING_PRECISION	2

#include "system_channels.h"


#endif /* CHANNELMETA_H_ */
