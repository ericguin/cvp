`timescale 1ns/1ps

module top(
	input [15:0] sw,
	output [15:0] led
);

	assign led = sw;

endmodule

module test();

	reg [15:0] sw;
	wire [15:0] led;

	top uut(.sw(sw),.led(led));

	initial begin
		$dumpvars(0,uut);
		sw = 16'h0000;
		#10;
		sw = 16'hFFFF;
		#10;
		$finish;
	end
endmodule
