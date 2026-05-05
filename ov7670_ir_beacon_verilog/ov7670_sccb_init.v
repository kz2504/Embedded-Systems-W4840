`timescale 1ns / 1ps

module ov7670_sccb_init #(
    parameter CLK_FREQ_HZ  = 24000000,
    parameter SCCB_FREQ_HZ = 100000,
    parameter DEVICE_ADDR  = 8'h42,
    parameter START_DELAY  = 24'd20000
)(
    input  wire clk,
    input  wire reset,
    input  wire start,

    output reg  sioc,
    output reg  siod_oe_low,
    output reg  busy,
    output reg  done,
    output reg  [7:0] rom_index
);

    localparam TICK_DIV = CLK_FREQ_HZ / (SCCB_FREQ_HZ * 4);

    localparam ST_IDLE       = 3'd0;
    localparam ST_DELAY      = 3'd1;
    localparam ST_LOAD       = 3'd2;
    localparam ST_START      = 3'd3;
    localparam ST_BITS       = 3'd4;
    localparam ST_STOP       = 3'd5;
    localparam ST_NEXT       = 3'd6;
    localparam ST_DONE       = 3'd7;

    reg [2:0]  state;
    reg [15:0] div_count;
    reg [1:0]  phase;
    reg [4:0]  bit_index;
    reg [23:0] delay_count;
    reg [7:0]  reg_addr_latched;
    reg [7:0]  reg_data_latched;

    wire [7:0] rom_reg_addr;
    wire [7:0] rom_reg_data;
    wire       rom_valid;

    ov7670_reg_rom rom_inst (
        .index(rom_index),
        .reg_addr(rom_reg_addr),
        .reg_data(rom_reg_data),
        .valid(rom_valid)
    );

    wire tick = div_count == TICK_DIV - 1;

    function [0:0] slot_bit;
        input [4:0] slot;
        begin
            if (slot < 5'd8) begin
                slot_bit = DEVICE_ADDR[7 - slot];
            end else if (slot < 5'd17) begin
                slot_bit = reg_addr_latched[16 - slot];
            end else begin
                slot_bit = reg_data_latched[25 - slot];
            end
        end
    endfunction

    function [0:0] slot_is_ack;
        input [4:0] slot;
        begin
            slot_is_ack = (slot == 5'd8) || (slot == 5'd17) || (slot == 5'd26);
        end
    endfunction

    always @(posedge clk) begin
        if (reset) begin
            div_count        <= 16'd0;
            phase            <= 2'd0;
            state            <= ST_IDLE;
            bit_index        <= 5'd0;
            delay_count      <= 24'd0;
            reg_addr_latched <= 8'd0;
            reg_data_latched <= 8'd0;
            sioc             <= 1'b1;
            siod_oe_low      <= 1'b0;
            busy             <= 1'b0;
            done             <= 1'b0;
            rom_index        <= 8'd0;
        end else begin
            if (tick) begin
                div_count <= 16'd0;
            end else begin
                div_count <= div_count + 1'b1;
            end

            case (state)
                ST_IDLE: begin
                    sioc        <= 1'b1;
                    siod_oe_low <= 1'b0;
                    busy        <= 1'b0;
                    done        <= 1'b0;
                    phase       <= 2'd0;
                    rom_index   <= 8'd0;
                    if (start) begin
                        busy        <= 1'b1;
                        delay_count <= 24'd0;
                        state       <= ST_DELAY;
                    end
                end

                ST_DELAY: begin
                    busy <= 1'b1;
                    if (tick) begin
                        if (delay_count == START_DELAY) begin
                            state <= ST_LOAD;
                        end else begin
                            delay_count <= delay_count + 1'b1;
                        end
                    end
                end

                ST_LOAD: begin
                    if (rom_valid) begin
                        reg_addr_latched <= rom_reg_addr;
                        reg_data_latched <= rom_reg_data;
                        phase            <= 2'd0;
                        state            <= ST_START;
                    end else begin
                        state <= ST_DONE;
                    end
                end

                ST_START: begin
                    if (tick) begin
                        phase <= phase + 1'b1;
                        case (phase)
                            2'd0: begin sioc <= 1'b1; siod_oe_low <= 1'b0; end
                            2'd1: begin sioc <= 1'b1; siod_oe_low <= 1'b1; end
                            2'd2: begin sioc <= 1'b0; siod_oe_low <= 1'b1; end
                            2'd3: begin
                                bit_index <= 5'd0;
                                phase     <= 2'd0;
                                state     <= ST_BITS;
                            end
                        endcase
                    end
                end

                ST_BITS: begin
                    if (tick) begin
                        phase <= phase + 1'b1;
                        case (phase)
                            2'd0: begin
                                sioc <= 1'b0;
                                if (slot_is_ack(bit_index)) begin
                                    siod_oe_low <= 1'b0;
                                end else begin
                                    siod_oe_low <= ~slot_bit(bit_index);
                                end
                            end
                            2'd1: begin sioc <= 1'b1; end
                            2'd2: begin sioc <= 1'b1; end
                            2'd3: begin
                                sioc <= 1'b0;
                                if (bit_index == 5'd26) begin
                                    phase <= 2'd0;
                                    state <= ST_STOP;
                                end else begin
                                    bit_index <= bit_index + 1'b1;
                                end
                            end
                        endcase
                    end
                end

                ST_STOP: begin
                    if (tick) begin
                        phase <= phase + 1'b1;
                        case (phase)
                            2'd0: begin sioc <= 1'b0; siod_oe_low <= 1'b1; end
                            2'd1: begin sioc <= 1'b1; siod_oe_low <= 1'b1; end
                            2'd2: begin sioc <= 1'b1; siod_oe_low <= 1'b0; end
                            2'd3: begin phase <= 2'd0; state <= ST_NEXT; end
                        endcase
                    end
                end

                ST_NEXT: begin
                    rom_index <= rom_index + 1'b1;
                    state     <= ST_LOAD;
                end

                ST_DONE: begin
                    busy        <= 1'b0;
                    done        <= 1'b1;
                    sioc        <= 1'b1;
                    siod_oe_low <= 1'b0;
                    if (!start) begin
                        state <= ST_IDLE;
                    end
                end
            endcase
        end
    end

endmodule
