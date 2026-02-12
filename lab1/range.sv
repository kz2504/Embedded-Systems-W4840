module range
   #(parameter
     RAM_WORDS = 16,            // Number of counts to store in RAM
     RAM_ADDR_BITS = 4)         // Number of RAM address bits
   (input logic         clk,    // Clock
    input logic 	go,     // Read start and start testing
    input logic [31:0] 	start,  // Number to start from or count to read
    output logic 	done,   // True once memory is filled
    output logic [15:0] count); // Iteration count once finished

   logic 		cgo;    // "go" for the Collatz iterator
   logic                cdone;  // "done" from the Collatz iterator
   logic [31:0] 	n;      // number to start the Collatz iterator

// verilator lint_off PINCONNECTEMPTY
   
   // Instantiate the Collatz iterator
   collatz c1(.clk(clk),
	      .go(cgo),
	      .n(n),
	      .done(cdone),
	      .dout());

   logic [RAM_ADDR_BITS - 1:0] 	 num;         // The RAM address to write
   logic 			 running; // True during the iterations

   /* Replace this comment and the code below with your solution,
      which should generate running, done, cgo, n, num, we, and din */
   logic reset_we;
   logic reset_done;
   logic creset;
   logic [RAM_ADDR_BITS:0] wr_count;

   assign we = !reset_we ? cdone : 1'b0;
   assign done = !reset_done ? (wr_count == (RAM_ADDR_BITS + 1)'(RAM_WORDS)) : 1'b0;
   
   always_ff @(posedge clk) begin
      if (go == 1'b1) begin
         running <= 1'b1;
         n <= start;
         num <= '0;
         din <= 16'b1;
         cgo <= 1'b1;
         wr_count <= '0;
         reset_done <= 1'b0;
         if (cgo == 1'b1) begin
            cgo <= 1'b0;
         end
      end else if (done == 1'b1) begin
         reset_done <= 1'b1;
         running <= 1'b0;
      end else if ((running == 1'b1) && (cdone == 1'b0)) begin
         reset_we <= 1'b0;
         cgo <= 1'b0;
         din <= din + 1;
      end else if ((running == 1'b1) && (cdone == 1'b1)) begin
         if (wr_count != (RAM_ADDR_BITS + 1)'(RAM_WORDS - 1)) begin
            cgo <= 1'b1;
         end
         if (cgo == 1'b1) begin
            cgo <= 1'b0;
         end
         if (creset == 1'b0) begin
            num <= num + 1;
            reset_we <= 1'b1;
            creset <= 1'b1;
            n <= n + 1;
            wr_count <= wr_count + 1;
         end else begin
            din <= 16'b1;
            creset <= 1'b0;
         end
      end
   end
   /* Replace this comment and the code above with your solution */
   
   logic 			 we;                    // Write din to addr
   logic [15:0] 		 din;                   // Data to write
   logic [15:0] 		 mem[RAM_WORDS - 1:0];  // The RAM itself
   logic [RAM_ADDR_BITS - 1:0] 	 addr;                  // Address to read/write

   assign addr = we ? num : start[RAM_ADDR_BITS-1:0];
   
   always_ff @(posedge clk) begin
      if (we) mem[addr] <= din;
      count <= mem[addr];      
   end

endmodule
	     
