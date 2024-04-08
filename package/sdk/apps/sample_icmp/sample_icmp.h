#ifndef SAMPLE_ICMP_H
#define SAMPLE_ICMP_H

#define PROGRAM "sample_icmp"
#define MAX_COUNT_TRIES 5

#ifndef DELAY_TIME
#define DELAY_TIME 1000
#endif

#define ENABLE_SLEEP

typedef enum _BOARD_STATE
{
  DISCONECTED = 0,
  CONNETED_FOR_SSID,
  INITED_IP,
  SENDING_ECHO,
  #ifdef ENABLE_SLEEP
  WAIT_REPLY,
  #endif
} BOARD_STATE;

#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE  32
#endif


#endif