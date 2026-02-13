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

   typedef enum {IDLE, RUNNING, DONE_LOCKED, N_VIRTUAL, TRIGGER} State;

   State state;

   //Debounce counter for key0/1
   logic [23:0] key0_counter; 
   logic [23:0] key1_counter; 

   //n value display
   logic [11:0] n_virtual;
   logic [11:0] n_hex;

   //Debounce counter for trigger
   logic [22:0] done_counter;

   logic [6:0] hex0;
   logic [6:0] hex1;
   logic [6:0] hex2;
   logic [6:0] hex3;
   logic [6:0] hex4;
   logic [6:0] hex5;

   hex7seg h5 (n_hex[11:8], hex5);
   hex7seg h4 (n_hex[7:4], hex4);
   hex7seg h3 (n_hex[3:0], hex3);

   hex7seg h2 (count[11:8], hex2);
   hex7seg h1 (count[7:4], hex1);
   hex7seg h0 (count[3:0], hex0);

   initial begin
      go = 1'b0;
      start = 32'b0;
      n = 12'b0;
      n_virtual = 12'b0;
      n_hex = 12'b0;

      state = IDLE;

      key0_counter = 24'b0;
      key1_counter = 24'b0;

      done_counter = 23'b0;

      LEDR = '0;

      hex0 = '0; hex1 = '0; hex2 = '0;
      hex3 = '0; hex4 = '0; hex5 = '0;
   end

   assign n_hex = (state == N_VIRTUAL) ? n_virtual : n; //mux outputs to first three 7-segs
   
   always_ff @(posedge clk) begin
      case (state)
         IDLE: begin //Idle/trigger ready
            n <= {2'b0, SW}; 
            if (KEY[2] == 1'b0) begin
               state <= IDLE; //KEY2 has precedence: do nothing
            end else if ((KEY[0] == 1'b0) || (KEY[1] == 1'b0)) begin
               state <= N_VIRTUAL; //Next, poll KEYS0 & 1: virtual mode
               n_virtual <= n;
            end else if (KEY[3] == 1'b0) begin
               state <= TRIGGER; //Finally, poll KEY3 for range trigger
            end
         end

         TRIGGER: begin //Configure and trigger range module
            go <= 1'b1;
            start <= {20'b0, n}; 
            state <= RUNNING;
         end

         N_VIRTUAL: begin
            key0_counter <= 24'b0; //Reset debounce counters
            key1_counter <= 24'b0;
            if ((KEY[2] == 1'b0) || (KEY[3] == 1'b0)) begin
               state <= TRIGGER; //Trigger has precedence
            end else if (KEY[0] == 1'b0) begin
               key0_counter <= key0_counter + 1'b1;
               if (key0_counter > 24'h989680) begin
                  if (n_virtual < n + 12'd255) begin
                     n_virtual <= n_virtual + 1'b1;
                     start <= {20'b0, 12'((n_virtual + 1'b1 - n))}; //Update count display with current value
                     //Note that n_virtual is the number to display but its address is its offset from n
                  end else if (n_virtual == n + 12'd255) begin
                     n_virtual <= n; //Wraparound
                     start <= {20'b0, n};
                  end
                  key0_counter <= 24'b0;
               end 
            end else if (KEY[1] == 1'b0) begin
               key1_counter <= key1_counter + 1'b1;
               if (key1_counter > 24'h989680) begin
                  if (n_virtual > n) begin 
                     n_virtual <= n_virtual - 1'b1;
                     start <= {20'b0, 12'((n_virtual - 1'b1 - n))}; //Update count display with current value
                  end else if (n_virtual == n) begin
                     n_virtual <= n + 12'd255; //Wraparound
                     start <= {20'b0, 12'd255};
                  end
                  key1_counter <= 24'b0;
               end 
            end else begin
               state <= N_VIRTUAL; //Stay in virtual n state unless triggered or KEY2 pressed
            end
         end

         RUNNING: begin //Range running: Do nothing
            LEDR <= '0;
            go <= 1'b0;
            if (done == 1'b1) begin
               state <= DONE_LOCKED; //Go to DONE_LOCKED if range sets done flag
            end else begin
               state <= RUNNING;
            end
         end

         DONE_LOCKED: begin //Range done: Wait for delay timer before arming again
            LEDR <= '1;
            start <= 32'b0;
            if (KEY[3] == 1'b1) begin
               done_counter <= done_counter + 1'b1;
               if (done_counter == '1) begin //Force delay before return to IDLE/trigger ready
                  state <= IDLE;
                  done_counter <= 23'b0;
               end
            end else begin
               state <= DONE_LOCKED;
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
