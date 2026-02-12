module collatz( input logic         clk,   // Clock
		input logic 	    go,    // Load value from n; start iterating
		input logic  [31:0] n,     // Start value; only read when go = 1
		output logic [31:0] dout,  // Iteration value: true after go = 1
		output logic 	    done); // True when dout reaches 1

   logic busy;
   logic [31:0] temp = 32'b0;
   assign temp = (dout % 2 == 1) ? 3 * dout + 1 : dout / 2;

   always_ff @(posedge clk) begin
      if (go == 1'b1) begin
         dout <= n;
         busy <= 1'b1;
         done <= 1'b0;
      end else if ((busy == 1'b1) && (done != 1'b1)) begin 
         dout <= temp;
         if (temp == 32'b1) begin
            busy <= 1'b0;
            done <= 1'b1;
         end
      end
   end

endmodule
