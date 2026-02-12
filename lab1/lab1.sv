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
 
   range #(16, 4) // RAM_WORDS = 256, RAM_ADDR_BITS = 8)
         r ( .* ); // Connect everything with matching names

   logic running; 
   logic key_latched;
   logic [6:0] hex0, hex1, hex2, hex3, hex4, hex5;

   hex7seg h5 (n[11:8], hex5);
   hex7seg h4 (n[7:4], hex4);
   hex7seg h3 (n[3:0], hex3);

   hex7seg h2 (count[11:8], hex2);
   hex7seg h1 (count[7:4], hex1);
   hex7seg h0 (count[3:0], hex0);

   initial begin
      go = 1'b0;
      start = 32'd0;
      n = 12'd0;

      running = 1'b0;
      key_latched = 1'b0;

      LEDR = '0;

      hex0 = '0; hex1 = '0; hex2 = '0;
      hex3 = '0; hex4 = '0; hex5 = '0;
   end

   always_ff @(posedge clk) begin
      go <= 1'b0;
      if (running == 1'b0) begin
         LEDR <= '1;
         n <= {2'b00, SW};
         if (KEY[3] == 1'b0) begin
            if (key_latched == 1'b0) begin
               go <= 1'b1;
               start <= {20'b0, n};
               running <= 1'b1;
               key_latched <= 1'b1;
            end
         end else begin
            key_latched <= 1'b0;
         end
      end else if (done == 1'b1) begin
         running <= 1'b0;
         start <= '0;
      end else begin
         LEDR <= '0;
      end
   end

   assign HEX0 = hex0;
   assign HEX1 = hex1;
   assign HEX2 = hex2;
   assign HEX3 = hex3;
   assign HEX4 = hex4;
   assign HEX5 = hex5;
   
endmodule
