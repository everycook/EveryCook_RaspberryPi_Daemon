double readTemp(struct Daemon_Values *dv);
double readPress(struct Daemon_Values *dv);
bool HeatOn(struct Daemon_Values *dv);
bool HeatOff(struct Daemon_Values *dv);
void setMotorPWM(uint16_t pwm, struct Daemon_Values *dv);
void setServoOpen(uint8_t openPercent, uint8_t steps, uint16_t stepWait, struct Daemon_Values *dv);
double readWeight(struct Daemon_Values *dv);
double readWeightSeparate(double* values, struct Daemon_Values *dv);

void SegmentDisplaySimple(char curSegmentDisplay, struct State *state, struct I2C_Config *i2c_config);
void SegmentDisplayOptimized(char curSegmentDisplay, struct State *state, struct I2C_Config *i2c_config);
void blink7Segment(struct I2C_Config *i2c_config);