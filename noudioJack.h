/*  noudioJack.c
 *
 *  This simple client demonstrates the basic features of JACK
 *  as they would be used by many applications.
 */
void noudioJackExit(void);
int  noudioJackInit(int doConnect);
void noudioJackGetBuf(float *out1, float *out2, int sz);
