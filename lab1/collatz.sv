module collatz( input logic         clk,   // Clock
		input logic 	    go,    // Load value from n; start iterating
		input logic  [31:0] n,     // Start value; only read when go = 1
		output logic [31:0] dout,  // Iteration value: true after go = 1
		output logic 	    done); // True when dout reaches 1

   logic busy;
   logic [31:0] next;

   initial begin
      busy = 1'b0;
      dout = 32'd0;
      done = 1'b0;
   end

   always_comb begin
         if (dout[0] == 1'b1) begin
            next = (32'b11 * dout) + 32'b1;
         end else begin 
            next = dout >> 1;
         end
   end

   always_ff @(posedge clk) begin
      if (go) begin
         dout <= n;
         if (n == 32'd1) begin
            busy <= 1'b0;
            done <= 1'b1;
         end else begin
            busy <= 1'b1;
            done <= 1'b0;
         end
      end else if (busy) begin
         dout <= next;
         if (next == 32'd1) begin
            busy <= 1'b0;
            done <= 1'b1;
         end
      end
   end
endmodule
