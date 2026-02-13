// CSEE 4840 Lab 1: Run and Display Collatz Conjecture Iteration Counts
//
// Spring 2023
//
// By: Leen Alshorafa, Kuan Zhang
// Uni: laa2202, kz2504

module lab1( input logic        CLOCK_50,  // 50 MHz Clock input
	     
	     input logic [3:0] 	KEY, // Pushbuttons; KEY[0] is rightmost

	     input logic [9:0] 	SW, // Switches; SW[0] is rightmost

	     // 7-segment LED displays; HEX0 is rightmost
	     output logic [6:0] HEX0, HEX1, HEX2, HEX3, HEX4, HEX5,

	     output logic [9:0] LEDR // LEDs above the switches; LED[0] on right
	     );

   logic 			clk, go, done;   
   logic [31:0] 		start;
   logic [15:0] 		count;

   logic [11:0] 		n;
   
   assign clk = CLOCK_50;
 
   range #(256, 8) // RAM_WORDS = 256, RAM_ADDR_BITS = 8)
         r ( .* ); // Connect everything with matching names

   typedef enum {IDLE, RUNNING, DONE_LOCKED, N_VIRTUAL} State;

   State state;

   logic [23:0] key0_counter; 
   logic [23:0] key1_counter; 

   logic [6:0] hex0, hex1, hex2, hex3, hex4, hex5;

   hex7seg h5 (n[11:8], hex5);
   hex7seg h4 (n[7:4], hex4);
   hex7seg h3 (n[3:0], hex3);

   hex7seg h2 (count[11:8], hex2);
   hex7seg h1 (count[7:4], hex1);
   hex7seg h0 (count[3:0], hex0);

   initial begin
      go = 1'b0;
      start = 32'b0;
      n = 12'b0;

      state = IDLE;

      key0_counter = 24'b0;
      key1_counter = 24'b0;

      LEDR = '0;

      hex0 = '0; hex1 = '0; hex2 = '0;
      hex3 = '0; hex4 = '0; hex5 = '0;
   end
   
   always_ff @(posedge clk) begin
      case (state)
         IDLE: begin
            n <= {2'b0, SW}; 
            if (KEY[2] == 1'b0) begin
               state <= IDLE; //Do nothing if KEY2 held
            end else if ((KEY[0] == 1'b0) || (KEY[1] == 1'b0)) begin
               state <= N_VIRTUAL;
            end else if (KEY[3] == 1'b0) begin
               start <= {20'b0, n};
               go <= 1'b1;
               state <= RUNNING;
            end
         end
         N_VIRTUAL: begin
            key0_counter <= 24'b0;
            key1_counter <= 24'b0;
            if (KEY[2] == 1'b0) begin
               state <= IDLE;
            end else if (KEY[0] == 1'b0) begin
               key0_counter <= key0_counter + 1'b1;
               if (key0_counter > 24'h989680) begin
                  n <= n + 1;
                  key0_counter <= 24'b0;
               end 
            end else if (KEY[1] == 1'b0) begin
               key1_counter <= key1_counter + 1'b1;
               if (key1_counter > 24'h989680) begin
                  n <= n - 1;
                  key1_counter <= 24'b0;
               end 
            end else if (KEY[3] == 1'b0) begin
               start <= {20'b0, n};
               go <= 1'b1;
               state <= RUNNING;
            end
         end
         RUNNING: begin
            LEDR <= '0;
            go <= 1'b0;
            if (done == 1'b1) begin
               state <= DONE_LOCKED;
            end else begin
               state <= RUNNING;
            end
         end
         DONE_LOCKED: begin
            LEDR <= '1;
            start <= 32'b0;
            state <= DONE_LOCKED;
            if (KEY[3] == 1'b1) begin
               state <= IDLE;
            end
         end
      endcase 
   end

   assign HEX0 = hex0;
   assign HEX1 = hex1;
   assign HEX2 = hex2;
   assign HEX3 = hex3;
   assign HEX4 = hex4;
   assign HEX5 = hex5;
   
endmodule
