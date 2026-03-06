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
#include <stdint.h>
#include <ctype.h>

/* Update SERVER_HOST to be the IP address of
 * the chat server you are connecting to
 */
/* arthur.cs.columbia.edu */
#define SERVER_HOST "128.59.19.114"
#define SERVER_PORT 42000

#define BUFFER_SIZE 128

#define SCREEN_ROWS 24
#define SCREEN_COLS 64
#define INPUT_ROWS 2
#define DIVIDER_ROW (SCREEN_ROWS - INPUT_ROWS - 1)
#define CHAT_ROWS DIVIDER_ROW
#define INPUT_START_ROW (DIVIDER_ROW + 1)
#define INPUT_MAX_CHARS (INPUT_ROWS * SCREEN_COLS)

/*
 * References:
 *
 * https://web.archive.org/web/20130307100215/http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 *
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

//Keyboard control logic
enum key_action {
  KEY_NONE = 0,
  KEY_INSERT_CHAR,
  KEY_ENTER,
  KEY_BACKSPACE,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_ESCAPE
};

//Struct for key press handling
struct key_event {
  enum key_action action;
  char ch;
};

static int sockfd; /* Socket file descriptor */
static struct libusb_device_handle *keyboard;
static uint8_t endpoint_address;

static pthread_t network_thread;
static void *network_thread_f(void *ignored);

static pthread_mutex_t screen_lock = PTHREAD_MUTEX_INITIALIZER;

static char input_buffer[INPUT_MAX_CHARS + 1];
static int input_len = 0;
static int cursor_pos = 0;
static int chat_next_row = 0;

static void clear_row_locked(int row);
static void draw_divider_locked(void);
static void init_screen(void);
static void render_input_locked(void);
static int next_chat_row_locked(int row);
static void chat_print_message(const char *message);
static int keycode_is_new(uint8_t keycode, const struct usb_keyboard_packet *previous);
static struct key_event decode_key_event(uint8_t keycode, uint8_t modifiers);
static void handle_key_event(const struct key_event *event);

//Clears row by writing spaces
static void clear_row_locked(int row)
{
  int col;
  for (col = 0; col < SCREEN_COLS; col++) {
    fbputchar(' ', row, col);
  }
}

//Draws line of '-' characters
static void draw_divider_locked(void)
{
  int col;
  for (col = 0; col < SCREEN_COLS; col++) {
    fbputchar('-', DIVIDER_ROW, col);
  }
}

static void render_input_locked(void)
{
  int row, col, i;
  int cursor_index;
  int cursor_row;
  int cursor_col;

  //Clears input box
  for (row = INPUT_START_ROW; row < SCREEN_ROWS; row++) {
    clear_row_locked(row);
  }

  //Render characters from input buffer with wraparound
  for (i = 0; i < input_len; i++) {
    row = INPUT_START_ROW + i / SCREEN_COLS;
    col = i % SCREEN_COLS;
    fbputchar(input_buffer[i], row, col);
  }

  //Prevents cursor overflow
  cursor_index = cursor_pos;
  if (cursor_index >= INPUT_MAX_CHARS) {
    cursor_index = INPUT_MAX_CHARS - 1;
  }
  //Render cursor with wraparound
  cursor_row = INPUT_START_ROW + cursor_index / SCREEN_COLS;
  cursor_col = cursor_index % SCREEN_COLS;
  draw_underline_cursor(cursor_row, cursor_col);
}

static void init_screen(void)
{
  int row;

  pthread_mutex_lock(&screen_lock); //Lock out network thread
  for (row = 0; row < SCREEN_ROWS; row++) {
    clear_row_locked(row); //Clear receive region
  }
  draw_divider_locked(); //Render divider
  render_input_locked(); //Init input region
  pthread_mutex_unlock(&screen_lock);
}

static int next_chat_row_locked(int row)
{
  int next = row + 1;
  if (next >= CHAT_ROWS) {
    next = 0; //Wraparound if at divider
  }
  clear_row_locked(next); //Prepare next row by clearing
  return next;
}

static void chat_print_message(const char *message)
{
  int row = chat_next_row;
  int col = 0;
  size_t i;
  size_t len;

  if (message == NULL) {
    return;
  }

  pthread_mutex_lock(&screen_lock); //Lock out network thread

  clear_row_locked(row);
  len = strlen(message);
  for (i = 0; i < len; i++) {
    unsigned char ch = (unsigned char)message[i];

    if (ch == '\r') { //Ignore carriage return
      continue; 
    }
    if (ch == '\n') { //Newline
      row = next_chat_row_locked(row); 
      col = 0;
      continue;
    }
    if (!isprint(ch)) { 
      ch = '?'; //Display '?' if unprintable
    }

    fbputchar((char)ch, row, col); //Render character
    col++;
    //Wraparound
    if (col >= SCREEN_COLS) {
      row = next_chat_row_locked(row);
      col = 0;
    }
  }

  //Print next message on next row unless we land back on next row first column
  if (col > 0) {
    chat_next_row = next_chat_row_locked(row); 
  } else {
    chat_next_row = row;
  }

  pthread_mutex_unlock(&screen_lock);
}

//Check if key press is new and not held
static int keycode_is_new(uint8_t keycode, const struct usb_keyboard_packet *previous)
{
  int i;
  for (i = 0; i < 6; i++) {
    if (previous->keycode[i] == keycode) {
      return 0;
    }
  }
  return 1;
}

//Produce a key_event struct upon key press to store pressed character and desired action
static struct key_event decode_key_event(uint8_t keycode, uint8_t modifiers)
{
  int shift = (modifiers & (USB_LSHIFT | USB_RSHIFT)) != 0; //Check shift
  struct key_event event;

  event.action = KEY_NONE;
  event.ch = 0;

  //Letter typed
  if (keycode >= 4 && keycode <= 29) {
    static const char *lower = "abcdefghijklmnopqrstuvwxyz";
    static const char *upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int idx = keycode - 4; //Shift keycode to index alphabet strings
    event.action = KEY_INSERT_CHAR; //
    event.ch = shift ? upper[idx] : lower[idx];
    return event;
  }

  //Number typed
  if (keycode >= 30 && keycode <= 39) {
    static const char normal[] = "1234567890";
    static const char shifted[] = "!@#$%^&*()";
    int idx = keycode - 30; //Shift keycode to index num/symbol strings
    event.action = KEY_INSERT_CHAR;
    event.ch = shift ? shifted[idx] : normal[idx];
    return event;
  }

  //Special character typed
  switch (keycode) {
  case 40:
    event.action = KEY_ENTER;
    break;
  case 41:
    event.action = KEY_ESCAPE;
    break;
  case 42:
    event.action = KEY_BACKSPACE;
    break;
  case 44:
    event.action = KEY_INSERT_CHAR;
    event.ch = ' ';
    break;
  case 45:
    event.action = KEY_INSERT_CHAR;
    event.ch = shift ? '_' : '-';
    break;
  case 46:
    event.action = KEY_INSERT_CHAR;
    event.ch = shift ? '+' : '=';
    break;
  case 47:
    event.action = KEY_INSERT_CHAR;
    event.ch = shift ? '{' : '[';
    break;
  case 48:
    event.action = KEY_INSERT_CHAR;
    event.ch = shift ? '}' : ']';
    break;
  case 49:
    event.action = KEY_INSERT_CHAR;
    event.ch = shift ? '|' : '\\';
    break;
  case 51:
    event.action = KEY_INSERT_CHAR;
    event.ch = shift ? ':' : ';';
    break;
  case 52:
    event.action = KEY_INSERT_CHAR;
    event.ch = shift ? '"' : '\'';
    break;
  case 53:
    event.action = KEY_INSERT_CHAR;
    event.ch = shift ? '~' : '`';
    break;
  case 54:
    event.action = KEY_INSERT_CHAR;
    event.ch = shift ? '<' : ',';
    break;
  case 55:
    event.action = KEY_INSERT_CHAR;
    event.ch = shift ? '>' : '.';
    break;
  case 56:
    event.action = KEY_INSERT_CHAR;
    event.ch = shift ? '?' : '/';
    break;
  case 79:
    event.action = KEY_RIGHT;
    break;
  case 80:
    event.action = KEY_LEFT;
    break;
  default:
    break;
  }

  return event;
}

//Perform action based on key_event struct
static void handle_key_event(const struct key_event *event)
{
  if (event == NULL || event->action == KEY_NONE || event->action == KEY_ESCAPE) {
    return; 
  }

  switch (event->action) {
  case KEY_INSERT_CHAR:
    if (input_len < INPUT_MAX_CHARS) { //Don't add if input region full
      //Shift text to the right of cursor position (inclusive) right by 1 in buffer to make space
      memmove(&input_buffer[cursor_pos + 1], &input_buffer[cursor_pos], (size_t)(input_len - cursor_pos));
      input_buffer[cursor_pos] = event->ch; //Insert character at cursor
      cursor_pos++; 
      input_len++;
      input_buffer[input_len] = '\0'; //Terminate input string
    }
    break;
  case KEY_BACKSPACE:
    if (cursor_pos > 0) {
      //Shift text to the right of cursor position (inclusive) left by 1, removing char left of cursor
      memmove(&input_buffer[cursor_pos - 1], &input_buffer[cursor_pos], (size_t)(input_len - cursor_pos));
      cursor_pos--;
      input_len--;
      input_buffer[input_len] = '\0'; //Terminate input string
    }
    break;
  case KEY_LEFT:
    if (cursor_pos > 0) {
      cursor_pos--; //Arrow keys: move cursor
    }
    break;
  case KEY_RIGHT:
    if (cursor_pos < input_len) {
      cursor_pos++; //Arrow keys: move cursor
    }
    break;
  case KEY_ENTER:
    if (input_len > 0) { //Only send if user inputted something
      //Write to socket and check return value for error
      if (write(sockfd, input_buffer, (size_t)input_len) < 0) {
        perror("Send failed");
      } (write(sockfd, "\n", 1) < 0) {
        perror("Send newline failed")
      } else {
        fprintf(stderr, "Send success\n");
      }
    }
    //Reset input box to empty
    input_len = 0;
    cursor_pos = 0;
    input_buffer[0] = '\0';
    break;
  case KEY_NONE:
  case KEY_ESCAPE:
    break;
  }

  pthread_mutex_lock(&screen_lock);
  render_input_locked(); //Update input region display
  pthread_mutex_unlock(&screen_lock);
}

int main(void)
{
  int err;
  int running = 1;
  
  struct sockaddr_in serv_addr;
  
  struct usb_keyboard_packet packet;
  struct usb_keyboard_packet previous_packet;
  int transferred = 0;
  char keystate[12];

  memset(&packet, 0, sizeof(packet));
  memset(&previous_packet, 0, sizeof(previous_packet));
  memset(input_buffer, 0, sizeof(input_buffer));

  if ((err = fbopen()) != 0) {
    fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
    exit(1);
  }
  init_screen();

  /* Open the keyboard */
  if ((keyboard = openkeyboard(&endpoint_address)) == NULL) {
    fprintf(stderr, "Did not find a keyboard\n");
    exit(1);
  }

  /* Create a TCP communications socket */
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "Error: Could not create socket\n");
    exit(1);
  }

  /* Get the server address */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);
  if (inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
    fprintf(stderr, "Error: Could not convert host IP \"%s\"\n", SERVER_HOST);
    exit(1);
  }

  /* Connect the socket to the server */
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "Error: connect() failed.  Is the server running?\n");
    exit(1);
  }

  /* Start the network thread */
  pthread_create(&network_thread, NULL, network_thread_f, NULL);

  /* Look for and handle keypresses */
  while (running) {
    libusb_interrupt_transfer(keyboard, endpoint_address,
			      (unsigned char *) &packet, sizeof(packet),
			      &transferred, 0);
    if (transferred == (int)sizeof(packet)) {
      // sprintf(keystate, "%02x %02x %02x", packet.modifiers, packet.keycode[0],
	    //   packet.keycode[1]);
      // printf("%s\n", keystate);

      for (int i = 0; i < 6; i++) { //Check all 6 keycodes in USB packet
        uint8_t keycode = packet.keycode[i];
        struct key_event event;

        if (keycode == 0 || !keycode_is_new(keycode, &previous_packet)) {
          continue; //Do nothing if no key or key is being held
        }

        event = decode_key_event(keycode, packet.modifiers); //Decode keypress
        if (event.action == KEY_ESCAPE) {
          running = 0;
          break; //Terminate if esc pressed
        }
        handle_key_event(&event); //Handle keypress
      }

      previous_packet = packet;
    }
  }

  shutdown(sockfd, SHUT_RDWR);
  close(sockfd);

  libusb_close(keyboard);
  libusb_exit(NULL);

  /* Terminate the network thread */
  pthread_cancel(network_thread);
  
  /* Wait for the network thread to finish */
  pthread_join(network_thread, NULL);

  return 0;
}

static void *network_thread_f(void *ignored)
{
  char recvBuf[BUFFER_SIZE];
  int n;

  while ((n = read(sockfd, recvBuf, BUFFER_SIZE - 1)) > 0) {
    recvBuf[n] = '\0';
    printf("%s", recvBuf);
    chat_print_message(recvBuf);
  }

  return NULL;
}
