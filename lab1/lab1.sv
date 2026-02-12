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

   // Replace this comment and the code below it with your own code;
   // The code below is merely to suppress Verilator lint warnings
   logic running; 
   logic [31:0] DEBOUNCE_CYCLES = 32'h100000; //~21 ms debounce timer
   logic [31:0] debounce_counter;
   logic key_latched;
   logic [6:0] hex0, hex1, hex2, hex3, hex4, hex5;
   logic [11:0] n_display;

   hex7seg h5 (n[11:8], hex5);
   hex7seg h4 (n[7:4], hex4);
   hex7seg h3 (n[3:0], hex3);

   hex7seg h2 (n_display[11:8], hex2);
   hex7seg h1 (n_display[7:4], hex1);
   hex7seg h0 (n_display[3:0], hex0);

   always_ff @(negedge clk) begin
      go <= 1'b0;
      if (running == 1'b0) begin
         n[9:0] <= SW;
         n[11:10] <= 2'b0;
         if (KEY[3] == 1'b0) begin
            if (debounce_counter < DEBOUNCE_CYCLES) begin 
               debounce_counter <= debounce_counter + 1;
            end else begin
               if (key_latched == 1'b0) begin
                  go <= 1'b1;
                  start[11:0] <= n;
                  running <= 1'b1;
                  key_latched <= 1'b1;
               end
            end
         end else begin
            debounce_counter <= 32'b0;
            key_latched <= 1'b0;
         end
      end else if (done == 1'b1) begin
         running <= 1'b0;
         start <= 32'b0;
         n_display <= count[11:0];
      end
   end

   assign HEX0 = hex0;
   assign HEX1 = hex1;
   assign HEX2 = hex2;
   assign HEX3 = hex3;
   assign HEX4 = hex4;
   assign HEX5 = hex5;
   assign LEDR = SW;
   
endmodule
