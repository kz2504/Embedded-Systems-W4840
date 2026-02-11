module hex7seg(input logic  [3:0] a,
	       output logic [6:0] y);

   always @(*) begin
      case (a)
          4'h0: y = 7'h40;
          4'h1: y = 7'h79;
          4'h2: y = 7'h24;
          4'h3: y = 7'h30;
          4'h4: y = 7'h19;
          4'h5: y = 7'h12;
          4'h6: y = 7'h02;
          4'h7: y = 7'h78;
          4'h8: y = 7'h00;
          4'h9: y = 7'h10;
          4'ha: y = 7'h08;
          4'hb: y = 7'h03;
          4'hc: y = 7'h46;
          4'hd: y = 7'h21;
          4'he: y = 7'h06;
          4'hf: y = 7'h0e;
          default: y = 7'hFF;
      endcase 
   end

endmodule
