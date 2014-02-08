#include "PWM_device.h"
#include "board.h"

///////////////////PWM Functions
static unsigned int MapPwmHardwareChannel(unsigned int softwareChannel){
	switch(softwareChannel){
	case 0:
		return 3;
		break;
	case 1:
		return 0;
		break;
	case 2:
		return 1;
		break;
	case 3:
	default:
		return 2;
		break;
	}
}

//------------------------------------------------------------------------------
/// Finds a prescaler/divisor couple to generate the desired frequency from
/// MCK.
/// Returns the value to enter in PWMC_MR or 0 if the configuration cannot be
/// met.
/// \param frequency  Desired frequency in Hz.
/// \param mck  Master clock frequency in Hz.
//------------------------------------------------------------------------------
static unsigned short PWM_GetClockConfiguration(
    unsigned int frequency,
    unsigned int mck)
{
    const unsigned int divisors[11] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
    unsigned char divisor = 0;
    unsigned int prescaler;

    // Find prescaler and divisor values
    prescaler = (mck / divisors[divisor]) / frequency;
    while ((prescaler > 255) && (divisor < 11)) {

        divisor++;
        prescaler = (mck / divisors[divisor]) / frequency;
    }

    // Return result
    if (divisor < 11) {
        return prescaler | (divisor << 8);
    }
    else {
        return 0;
    }
}

//------------------------------------------------------------------------------
/// Configures PWM clocks A & B to run at the given frequencies. This function
/// finds the best MCK divisor and prescaler values automatically.
/// \param clka  Desired clock A frequency (0 if not used).
/// \param clkb  Desired clock B frequency (0 if not used).
/// \param mck  Master clock frequency.
//------------------------------------------------------------------------------
static void PWM_ConfigureClocks(unsigned int clka, unsigned int clkb, unsigned int mck){

    unsigned int mode = 0;
    unsigned int result;

    // Clock A
    if (clka != 0) {
        result = PWM_GetClockConfiguration(clka, mck);
        mode |= result;
    }

    // Clock B
    if (clkb != 0) {
        result = PWM_GetClockConfiguration(clkb, mck);
        mode |= (result << 16);
    }

    // Configure clocks
    AT91C_BASE_PWMC->PWMC_MR = mode;
}

//------------------------------------------------------------------------------
/// Configures PWM a channel with the given parameters.
/// The PWM controller must have been clocked in the PMC prior to calling this
/// function.
/// \param channel  Channel number.
/// \param prescaler  Channel prescaler.
/// \param alignment  Channel alignment.
/// \param polarity  Channel polarity.
//------------------------------------------------------------------------------
static void PWM_ConfigureChannel(
    unsigned int channel,
    unsigned int prescaler,
    unsigned int alignment,
    unsigned int polarity)
{
	unsigned int hardwareChannel = MapPwmHardwareChannel(channel);
    // Disable channel
    AT91C_BASE_PWMC->PWMC_DIS = 1 << hardwareChannel;
    // Configure channel
    AT91C_BASE_PWMC->PWMC_CH[hardwareChannel].PWMC_CMR = prescaler | alignment | polarity;
}

static void PWM_configure_ports(){
	//Configure PWM ports
	/////////////////////////////////////////
	//PWM0
	/////////////////////////////////////////
 	//Configure Peripherials for PWM outputs
	AT91F_PIO_CfgPeriph(AT91C_BASE_PIOA,
			0, 				// mux function A
			AT91C_PIO_PA23); // mux funtion B

	/////////////////////////////////////////
	//PWM1
	/////////////////////////////////////////
 	//Configure Peripherials for PWM outputs
	AT91F_PIO_CfgPeriph(AT91C_BASE_PIOA,
			0, 				// mux function A
			AT91C_PIO_PA24); // mux funtion B

	/////////////////////////////////////////
	//PWM2
	/////////////////////////////////////////
 	//Configure Peripherials for PWM outputs
	AT91F_PIO_CfgPeriph(AT91C_BASE_PIOA,
			0, 				// mux function A
			AT91C_PIO_PA25); // mux funtion B

	/////////////////////////////////////////
	//PWM3
	/////////////////////////////////////////
 	//Configure Peripherials for PWM outputs
	AT91F_PIO_CfgPeriph(AT91C_BASE_PIOA,
			0, 				// mux function A
			AT91C_PIO_PA7); // mux funtion B
}

void PWM_configure_clock(unsigned short clockFrequency){
	PWM_ConfigureClocks(clockFrequency * MAX_PWM_DUTY_CYCLE, 0, BOARD_MCK);
}

void PWM_device_channel_init(unsigned int channel, unsigned short period, unsigned short dutyCycle){
    // Configure PWMC channel (left-aligned)
    PWM_ConfigureChannel(channel, AT91C_PWMC_CPRE_MCKA, 0, AT91C_PWMC_CPOL);
    PWM_SetPeriod(channel, period);
    PWM_SetDutyCycle(channel, dutyCycle);
    StartPWM(channel);
}

void PWM_device_init(){
	PWM_configure_ports();
	StartAllPWM();
}

void StartAllPWM(){
	AT91F_PWMC_StartChannel(AT91C_BASE_PWMC,(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3));
}

void StopAllPWM(){
	AT91F_PWMC_StopChannel(AT91C_BASE_PWMC,(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3));
}

void StartPWM(unsigned int channel){
	AT91F_PWMC_StartChannel(AT91C_BASE_PWMC,1 << channel);
}

void StopPWM(unsigned int channel){
	if (channel <= 3) AT91F_PWMC_StopChannel(AT91C_BASE_PWMC,1 << channel);
}

//------------------------------------------------------------------------------
/// Sets the period value used by a PWM channel. This function writes directly
/// to the CPRD register if the channel is disabled; otherwise, it uses the
/// update register CUPD.
/// \param channel  Channel number.
/// \param period  Period value.
//------------------------------------------------------------------------------
void PWM_SetPeriod(unsigned int channel, unsigned short period)
{
	unsigned int hardwareChannel = MapPwmHardwareChannel(channel);
    // If channel is disabled, write to CPRD
    if ((AT91C_BASE_PWMC->PWMC_SR & (1 << hardwareChannel)) == 0) {
        AT91C_BASE_PWMC->PWMC_CH[hardwareChannel].PWMC_CPRDR = period;
    }
    // Otherwise use update register
    else {
        AT91C_BASE_PWMC->PWMC_CH[hardwareChannel].PWMC_CMR |= AT91C_PWMC_CPD;
        AT91C_BASE_PWMC->PWMC_CH[hardwareChannel].PWMC_CUPDR = period;
    }
}

unsigned short PWM_GetPeriod(unsigned int channel){
	unsigned int hardwareChannel = MapPwmHardwareChannel(channel);
	return AT91C_BASE_PWMC->PWMC_CH[hardwareChannel].PWMC_CPRDR;
}

//------------------------------------------------------------------------------
/// Sets the duty cycle used by a PWM channel. This function writes directly to
/// the CDTY register if the channel is disabled; otherwise it uses the
/// update register CUPD.
/// Note that the duty cycle must always be inferior or equal to the channel
/// period.
/// \param channel  Channel number.
/// \param duty  Duty cycle value.
//------------------------------------------------------------------------------
void PWM_SetDutyCycle(unsigned int channel, unsigned short duty){

	unsigned int hardwareChannel = MapPwmHardwareChannel(channel);
	//duty cycle of zero freaks out the PWM controller
	//seems to invert the polarity. fix this up until we understand better.
	if (duty < 1 ) duty = 1;
	if (duty > 100) duty = 100;

    // If channel is disabled, write to CDTY
    if ((AT91C_BASE_PWMC->PWMC_SR & (1 << hardwareChannel)) == 0) {
        AT91C_BASE_PWMC->PWMC_CH[hardwareChannel].PWMC_CDTYR = duty;
    }
    // Otherwise use update register
    else {
        AT91C_BASE_PWMC->PWMC_CH[hardwareChannel].PWMC_CMR &= ~AT91C_PWMC_CPD;
        AT91C_BASE_PWMC->PWMC_CH[hardwareChannel].PWMC_CUPDR = duty;
    }
}

unsigned short PWM_GetDutyCycle(unsigned int channel){
	unsigned int hardwareChannel = MapPwmHardwareChannel(channel);
	return AT91C_BASE_PWMC->PWMC_CH[hardwareChannel].PWMC_CDTYR;
}


