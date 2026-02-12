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
   logic 			 running = 0; // True during the iterations

   logic [2:0] state = 3'b0;
   logic [2:0] next_state = 3'b0;

   always_comb begin
      we = 1'b0;
      case (state)
         3'b000: begin //Idle
            if (go == 1'b1) begin
               next_state = 3'b001;
            end else begin
               next_state = state;
            end
         end
         3'b001: begin //Go, set cgo
            next_state = 3'b010; 
         end
         3'b010: begin //Reset cgo
            next_state = 3'b011;
         end
         3'b011: begin //Increment din, wait for cdone
            if (cdone == 1'b1) begin
               next_state = 3'b100;
               we = 1'b1;
            end else begin
               next_state = state;
            end
         end
         3'b100: begin 
            we = 1'b0;
         end

      endcase
   end 

   /* Replace this comment and the code below with your solution,
      which should generate running, done, cgo, n, num, we, and din */
   always_ff @(posedge clk) begin
      case (next_state)
      3'b001: begin
         running <= 1'b1;
         n <= start;
         num <= '0;
         din <= 1'b1;
         cgo <= 1'b1;
      end
      3'b010: begin
         cgo <= 1'b0;
      end 
      3'b011: begin
         din <= din + 1;
      end
      3'b100: begin
         cgo <= 1'b1;
      end 
      endcase 
   end
   state <= next_state;
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
	     
