/*
 *
 * CSEE 4840 Lab 2 for 2019
 *
 * Name/UNI: Please Changeto Yourname (pcy2301)
 */
#include "fbputchar.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "usbkeyboard.h"

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
#define RECV_TOP_ROW 0
#define RECV_BOTTOM_ROW (DIVIDER_ROW - 1)
#define INPUT_TOP_ROW (DIVIDER_ROW + 1)

/* Visible input area capacity: 2 rows minus prompt characters */
#define INPUT_PROMPT "> "
#define INPUT_CAPACITY ((SCREEN_COLS * INPUT_ROWS) - 2)

int sockfd; /* Socket file descriptor */

struct libusb_device_handle *keyboard;
uint8_t endpoint_address;

pthread_t network_thread;
void *network_thread_f(void *);

static pthread_mutex_t fb_lock = PTHREAD_MUTEX_INITIALIZER;

static char input_buf[BUFFER_SIZE];
static int input_len = 0;
static int cursor_pos = 0;

static int recv_row = RECV_TOP_ROW;
static int recv_col = 0;

static int key_in_packet(const struct usb_keyboard_packet *packet, uint8_t key)
{
  int i;
  for (i = 0; i < 6; i++) {
    if (packet->keycode[i] == key) {
      return 1;
    }
  }
  return 0;
}

static void clear_row(int row)
{
  int col;
  for (col = 0; col < SCREEN_COLS; col++) {
    fbputchar(' ', row, col);
  }
}

static void clear_region(int r0, int r1)
{
  int row;
  for (row = r0; row <= r1; row++) {
    clear_row(row);
  }
}

static void draw_layout(void)
{
  int col;

  pthread_mutex_lock(&fb_lock);

  clear_region(0, SCREEN_ROWS - 1);

  for (col = 0; col < SCREEN_COLS; col++) {
    fbputchar('-', DIVIDER_ROW, col);
  }

  fbputs("Received messages", 0, 0);
  fbputs("Type and press Enter:", INPUT_TOP_ROW, 0);

  pthread_mutex_unlock(&fb_lock);
}

static void render_input(void)
{
  int visible_start = 0;
  int i;
  int prompt_cols = 2;
  int cursor_in_visible;

  pthread_mutex_lock(&fb_lock);

  clear_region(INPUT_TOP_ROW, SCREEN_ROWS - 1);
  fbputs("Type and press Enter:", INPUT_TOP_ROW, 0);
  fbputs(INPUT_PROMPT, INPUT_TOP_ROW + 1, 0);

  if (cursor_pos >= INPUT_CAPACITY) {
    visible_start = cursor_pos - INPUT_CAPACITY + 1;
  }

  for (i = visible_start; i < input_len && (i - visible_start) < INPUT_CAPACITY; i++) {
    int screen_index = prompt_cols + (i - visible_start);
    int row = INPUT_TOP_ROW + (screen_index / SCREEN_COLS);
    int col = screen_index % SCREEN_COLS;
    fbputchar(input_buf[i], row, col);
  }

  cursor_in_visible = cursor_pos - visible_start;
  if (cursor_in_visible < 0) {
    cursor_in_visible = 0;
  }
  if (cursor_in_visible >= INPUT_CAPACITY) {
    cursor_in_visible = INPUT_CAPACITY - 1;
  }

  {
    int cursor_screen_index = prompt_cols + cursor_in_visible;
    int cursor_row = INPUT_TOP_ROW + (cursor_screen_index / SCREEN_COLS);
    int cursor_col = cursor_screen_index % SCREEN_COLS;
    fbputchar('_', cursor_row, cursor_col);
  }

  pthread_mutex_unlock(&fb_lock);
}

static void advance_recv_line(void)
{
  recv_row++;
  recv_col = 0;

  if (recv_row > RECV_BOTTOM_ROW) {
    recv_row = RECV_TOP_ROW;
    clear_region(RECV_TOP_ROW, RECV_BOTTOM_ROW);
    fbputs("Received messages", 0, 0);
  }
}

static void append_received_text(const char *text)
{
  int i;

  pthread_mutex_lock(&fb_lock);
  for (i = 0; text[i] != '\0'; i++) {
    if (text[i] == '\n' || text[i] == '\r') {
      advance_recv_line();
      continue;
    }

    fbputchar(text[i], recv_row, recv_col);
    recv_col++;

    if (recv_col >= SCREEN_COLS) {
      advance_recv_line();
    }
  }
  advance_recv_line();
  pthread_mutex_unlock(&fb_lock);
}

enum key_action {
  KEY_NONE,
  KEY_CHAR,
  KEY_ENTER,
  KEY_BACKSPACE,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_ESC
};

struct key_event {
  enum key_action action;
  char ch;
};

static struct key_event decode_key(uint8_t modifiers, uint8_t keycode)
{
  struct key_event ev = {KEY_NONE, 0};
  int shifted = (modifiers & USB_LSHIFT) || (modifiers & USB_RSHIFT);

  if (keycode >= 0x04 && keycode <= 0x1d) {
    ev.action = KEY_CHAR;
    ev.ch = (shifted ? 'A' : 'a') + (keycode - 0x04);
    return ev;
  }

  if (keycode >= 0x1e && keycode <= 0x27) {
    static const char normal[] = "1234567890";
    static const char shifted_chars[] = "!@#$%^&*()";
    ev.action = KEY_CHAR;
    ev.ch = shifted ? shifted_chars[keycode - 0x1e] : normal[keycode - 0x1e];
    return ev;
  }

  switch (keycode) {
  case 0x28:
    ev.action = KEY_ENTER;
    break;
  case 0x29:
    ev.action = KEY_ESC;
    break;
  case 0x2a:
    ev.action = KEY_BACKSPACE;
    break;
  case 0x2c:
    ev.action = KEY_CHAR;
    ev.ch = ' ';
    break;
  case 0x2d:
    ev.action = KEY_CHAR;
    ev.ch = shifted ? '_' : '-';
    break;
  case 0x2e:
    ev.action = KEY_CHAR;
    ev.ch = shifted ? '+' : '=';
    break;
  case 0x2f:
    ev.action = KEY_CHAR;
    ev.ch = shifted ? '{' : '[';
    break;
  case 0x30:
    ev.action = KEY_CHAR;
    ev.ch = shifted ? '}' : ']';
    break;
  case 0x31:
    ev.action = KEY_CHAR;
    ev.ch = shifted ? '|' : '\\';
    break;
  case 0x33:
    ev.action = KEY_CHAR;
    ev.ch = shifted ? ':' : ';';
    break;
  case 0x34:
    ev.action = KEY_CHAR;
    ev.ch = shifted ? '"' : '\'';
    break;
  case 0x35:
    ev.action = KEY_CHAR;
    ev.ch = shifted ? '~' : '`';
    break;
  case 0x36:
    ev.action = KEY_CHAR;
    ev.ch = shifted ? '<' : ',';
    break;
  case 0x37:
    ev.action = KEY_CHAR;
    ev.ch = shifted ? '>' : '.';
    break;
  case 0x38:
    ev.action = KEY_CHAR;
    ev.ch = shifted ? '?' : '/';
    break;
  case 0x4f:
    ev.action = KEY_RIGHT;
    break;
  case 0x50:
    ev.action = KEY_LEFT;
    break;
  default:
    ev.action = KEY_NONE;
    break;
  }

  return ev;
}

static void input_insert_char(char c)
{
  int i;
  if (input_len >= BUFFER_SIZE - 1) {
    return;
  }

  for (i = input_len; i > cursor_pos; i--) {
    input_buf[i] = input_buf[i - 1];
  }

  input_buf[cursor_pos] = c;
  input_len++;
  cursor_pos++;
}

static void input_backspace(void)
{
  int i;
  if (cursor_pos == 0 || input_len == 0) {
    return;
  }

  for (i = cursor_pos - 1; i < input_len - 1; i++) {
    input_buf[i] = input_buf[i + 1];
  }

  cursor_pos--;
  input_len--;
}

static void input_submit(void)
{
  char sendbuf[BUFFER_SIZE + 6];

  if (input_len <= 0) {
    return;
  }

  input_buf[input_len] = '\0';

  if (write(sockfd, input_buf, input_len) < 0) {
    perror("write");
  }

  snprintf(sendbuf, sizeof(sendbuf), "me: %s", input_buf);
  append_received_text(sendbuf);

  input_len = 0;
  cursor_pos = 0;
}

int main()
{
  int err;

  struct sockaddr_in serv_addr;

  struct usb_keyboard_packet packet;
  struct usb_keyboard_packet previous_packet;
  int transferred;
  int i;

  memset(&previous_packet, 0, sizeof(previous_packet));

  if ((err = fbopen()) != 0) {
    fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
    exit(1);
  }

  draw_layout();
  render_input();

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
  for (;;) {
    libusb_interrupt_transfer(keyboard, endpoint_address,
                              (unsigned char *)&packet, sizeof(packet),
                              &transferred, 0);
    if (transferred == sizeof(packet)) {
      for (i = 0; i < 6; i++) {
        uint8_t keycode = packet.keycode[i];
        struct key_event ev;

        if (keycode == 0) {
          continue;
        }

        if (key_in_packet(&previous_packet, keycode)) {
          continue;
        }

        ev = decode_key(packet.modifiers, keycode);
        switch (ev.action) {
        case KEY_CHAR:
          input_insert_char(ev.ch);
          render_input();
          break;
        case KEY_BACKSPACE:
          input_backspace();
          render_input();
          break;
        case KEY_LEFT:
          if (cursor_pos > 0) {
            cursor_pos--;
            render_input();
          }
          break;
        case KEY_RIGHT:
          if (cursor_pos < input_len) {
            cursor_pos++;
            render_input();
          }
          break;
        case KEY_ENTER:
          input_submit();
          render_input();
          break;
        case KEY_ESC:
          goto done;
        case KEY_NONE:
        default:
          break;
        }
      }

      previous_packet = packet;
    }
  }

done:
  /* Terminate the network thread */
  pthread_cancel(network_thread);

  /* Wait for the network thread to finish */
  pthread_join(network_thread, NULL);

  return 0;
}

void *network_thread_f(void *ignored)
{
  char recv_buf[BUFFER_SIZE];
  int n;
  (void)ignored;

  /* Receive data */
  while ((n = read(sockfd, &recv_buf, BUFFER_SIZE - 1)) > 0) {
    recv_buf[n] = '\0';
    append_received_text(recv_buf);
  }

  return NULL;
}
