// -*- C -*-

/* Copyright 2015 by Chris Osborn <fozztexx@fozztexx.com>
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#include <IRremote.h>

#define BUFLEN		150

#define XMIT_PIN	3
#define RECV_PIN	11
#define WIRED_PIN	12
#define LED_PIN		13

#define STATE_IRON	1
#define STATE_IROFF	0
#define STATE_SERIAL	2

IRsend irsend;

unsigned long last, ser_last;
int state = 0;
unsigned int rcv_buf[BUFLEN], rcv_pos = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(RECV_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(WIRED_PIN, OUTPUT);
  last = micros();
  irsend.enableIROut(38);
}

void transmit_IR()
{
  unsigned int i, on;
  int len;
  

  for (i = 0; i < rcv_pos; i++) {
    on = !(i % 2);
    len = rcv_buf[i];
    len *= 2 * on - 1;
    digitalWrite(LED_PIN, on);
    digitalWrite(WIRED_PIN, on);
    if (on)
      irsend.mark(rcv_buf[i] * 100);
    else
      irsend.space(rcv_buf[i] * 100);
  }
  
  digitalWrite(LED_PIN, 0);
  digitalWrite(WIRED_PIN, 0);
  irsend.space(1);

  return;
}

void loop()
{
  unsigned long now, len;
  int cur;
  unsigned int i;
  
  
  now = micros();

  while (Serial.available()) {
    if (state != STATE_SERIAL) {
      rcv_pos = 0;
      state = STATE_SERIAL;
      rcv_buf[rcv_pos] = 0;
    }
    
    cur = Serial.read();
    ser_last = now;
    if (cur >= '0' && cur <= '9') {
      rcv_buf[rcv_pos] *= 10;
      rcv_buf[rcv_pos] += cur - '0';
    }
    else if (cur == ',') {
      rcv_pos++;
      rcv_buf[rcv_pos] = 0;
    }

    if (rcv_pos == BUFLEN || cur == '\n' || cur == '\r') {
      rcv_pos++;
      if (rcv_buf[0] || rcv_pos > 1)
	transmit_IR();
      rcv_pos = 0;
      rcv_buf[rcv_pos] = 0;

      /* Still receiving if we filled the buffer */
      if (cur == '\n' || cur == '\r') {
	state = STATE_IROFF;
	Serial.print("READY\r\n");
      }
    }
  }

  if (state == STATE_SERIAL && now - ser_last >= 10000000) {
    Serial.print("TIMEOUT\r\n");
    rcv_pos = 0;
    state = STATE_IROFF;
  }

  if (state != STATE_SERIAL) {
    len = now - last;
    cur = !digitalRead(RECV_PIN);
    digitalWrite(LED_PIN, cur);
  
    if (!cur && len > 100000 && rcv_pos) {
      if (rcv_pos > 10) {
	for (i = 0; i < rcv_pos; i++) {
	  if (i)
	    Serial.print(",");
	  Serial.print(rcv_buf[i]);
	}
	Serial.print("\r\n");
      }

      rcv_pos = 0;
    }
  
    if (cur != state) {
      if (rcv_pos < BUFLEN && (rcv_pos || state))
	rcv_buf[rcv_pos++] = len / 100;
      last = now;
      state = cur;
    }
  }
  
  return;
}
