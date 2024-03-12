#ifndef SAMPLE_ICMP_H
#define SAMPLE_ICMP_H

#define PROGRAM "sample_icmp"
#define MAX_COUNT_TRIES 5

typedef enum _BOARD_STATE
{
  DISCONECTED = 0,
  CONNETED_FOR_SSID,
  INITED_IP,
  SEND_ECHO,
  RECIVE_ECHO
} BOARD_STATE;


#endif