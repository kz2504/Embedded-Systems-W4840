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
   //logic 			 running; // True during the iterations

   typedef enum {IDLE, RESET_CGO, RUNNING, DONE} State;
   State state;
   logic reset_we;
   logic [RAM_ADDR_BITS:0] write_counter;

   initial begin
      //running = 1'b0;
      cgo = 1'b0;
      n = 32'd0;
      num = '0;

      reset_we = 1'b0;
      write_counter = '0;

      din = 16'd0;
      count = 16'd0;
      state = IDLE;
   end

   assign we = !reset_we ? cdone : 1'b0;
   
   always_ff @(posedge clk) begin
      case (state) 
         IDLE: begin
            if (go) begin
               state <= RESET_CGO;
               //running <= 1'b1; //Set running
               n <= start; //Load start into n
               num <= '0; //Reset num/RAM addr
               din <= 16'b1; //Set din to 1
               cgo <= 1'b1; //Begin cgo pulse
            end else begin
               state <= IDLE;
            end
         end
         RESET_CGO: begin
            din <= 16'b1; //Ensure din at 1
            cgo <= 1'b0; //End cgo pulse
            reset_we <= 1'b0; //Arm we
            state <= RUNNING;
         end
         RUNNING: begin
            din <= din + 1'b1;
            if (cdone) begin
               num <= num + 1'b1; //Increment write address
               reset_we <= 1'b1; //Reset we 
               write_counter <= write_counter + 1'b1;
               if (write_counter < RAM_WORDS - 1) begin
                  n <= n + 1'b1; //Increment n
                  cgo <= 1'b1; //Start collatz again
                  state <= RESET_CGO;
               end else begin
                  state <= DONE;
                  write_counter <= '0; 
                  //running <= 1'b0; //Reset running
                  done <= 1'b1; //Raise done
               end
            end else begin
               state <= RUNNING;
            end
         end 
         DONE: begin
            done <= 1'b0; //End done pulse
            state <= IDLE; //Go back to IDLE
         end
         
         default: state <= IDLE;
      endcase
   end
   
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
	     
