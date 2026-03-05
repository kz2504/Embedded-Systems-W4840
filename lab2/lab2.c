/*
 *
 CS 4840 Lab 2 for 2019
 *
 * Name/UNI: Leen Alshorafa (laa2202) - Kuan Zhang (kz2504)
 */
#include "fbputchar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "usbkeyboard.h"
#include <pthread.h>

char input_buffer[128];
int cursor_pos = 0; 
int chat_row = 0; 

char keycode_to_ascii(int code, int shift) {
	
	if(code >= 4 && code <= 29) {
		char c = 'a' + (code - 4); 
		if (shift) c -= 32; 
		return c; 
	} 


	if (code >= 30 && code <=38)
		return '1' + (code - 30); 

	if (code == 44)
		return ' '; 

	if (code == 40)
		return '\n'; 

	if (code == 42)
		return '\b';


	return 0; 

} 

/* Update SERVER_HOST to be the IP address of
 * the chat server you are connecting to
 */
/* arthur.cs.columbia.edu */
#define SERVER_HOST "128.59.19.114"
#define SERVER_PORT 42000
#define SCREEN_ROWS 24
#define SCREEN_COLS 64
#define INPUT_ROWS 2
#define CHAT_ROWS (SCREEN_ROWS - INPUT_ROWS - 1) 

#define BUFFER_SIZE 128

/*
 * References:
 *
 * https://web.archive.org/web/20130307100215/http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 *
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

int sockfd; /* Socket file descriptor */

struct libusb_device_handle *keyboard;
uint8_t endpoint_address;

pthread_t network_thread;
void *network_thread_f(void *);

int main()
{
  int err, col;

  struct sockaddr_in serv_addr;

  struct usb_keyboard_packet packet;
  int transferred;
  char keystate[12];

  if ((err = fbopen()) != 0) {
    fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
    exit(1);
  }

  /* Open the keyboard */
  if ( (keyboard = openkeyboard(&endpoint_address)) == NULL ) {
    fprintf(stderr, "Did not find a keyboard\n");
    exit(1);
  }
    
  /* Create a TCP communications socket */
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    fprintf(stderr, "Error: Could not create socket\n");
    exit(1);
  }

  /* Get the server address */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);
  if ( inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
    fprintf(stderr, "Error: Could not convert host IP \"%s\"\n", SERVER_HOST);
    exit(1);
  }

  /* Connect the socket to the server */
  if ( connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "Error: connect() failed.  Is the server running?\n");
    exit(1);
  }

  /* Start the network thread */
  pthread_create(&network_thread, NULL, network_thread_f, NULL);

  /* Look for and handle keypresses */
  for (;;) {
    libusb_interrupt_transfer(keyboard, endpoint_address,
			      (unsigned char *) &packet, sizeof(packet),
			      &transferred, 0);
    if (transferred == sizeof(packet)) {
      if (packet.keycode[0] == 0x29) { /* ESC pressed? */
	break;
      }

	int keycode = packet.keycode[0]; 
	int shift = packet.modifiers & 0x22; 

	char c = keycode_to_ascii(keycode, shift); 
	
	if (c) {
		
		if (c == '\n') {
			
			write(sockfd, input_buffer, strlen(input_buffer));

			fbputs(input_buffer, 0, chat_line++); 
			cursor_pos = 0; 
			memset(input_buffer, 0, sizeof(input_buffer)); 
		} 

		else if (c == '\b' && cursor_pos > 0) {

		cursor_pos--; 
		input_buffer[cursor_pos] = 0; 

		fbputchar(' ', cursor_pos, 35); 
	} 

		else {
			input_buffer[cursor_pos++] = c;
			fbputchar(c, cursor_pos, 35); 
	} 


    }
  }

} 

  /* Terminate the network thread */
  pthread_cancel(network_thread);

  /* Wait for the network thread to finish */
  pthread_join(network_thread, NULL);

  return 0;
}

void *network_thread_f(void *ignored)
{
  char recvBuf[BUFFER_SIZE];
  int n;
  /* Receive data */ 

  while ( (n = read(sockfd, recvBuf, BUFFER_SIZE - 1)) > 0 ) {
    recvBuf[n] = '\0';
    printf("%s", recvBuf);
    fbputs(recvBuf, 0, chat_line++); 
    
    if(chat_line > 30)
	chat_line = 0; 
  }

  return NULL;
}

