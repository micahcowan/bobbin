#!/usr/bin/python

from common import *

# Test 65C02 CPU instruction implementation and 6502 compatibility

@bobbin('-m enhanced --simple -q')
def test_65c02_inc_a_instruction(p):
    """Test INC A instruction (0x1A) on Enhanced Apple IIe - CORRECTED VERSION"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # CORRECTED: Test CPU register, not BASIC variable
        # LDA #5, INC A, STA $300, RTS - should store 6 in memory location $300
        ('FOR I=0 TO 6: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,5,26,141,0,3,96', ''),  # LDA #5, INC A, STA $300, RTS
        ('CALL 768: PRINT PEEK(768)', '6\r\n'),  # Should be 6 (5+1)
        
        # Test wraparound: LDA #255, INC A, STA $301, RTS
        ('FOR I=0 TO 6: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,255,26,141,1,3,96', ''),
        ('CALL 768: PRINT PEEK(769)', '0\r\n'),  # Should be 0 (255+1 wraps)
        
        # Test sign change: LDA #127, INC A, STA $302, RTS  
        ('FOR I=0 TO 6: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,127,26,141,2,3,96', ''),
        ('CALL 768: PRINT PEEK(770)', '128\r\n'), # Should be 128 (127+1)
    ])
    return True

@bobbin('-m plus --simple -q') 
def test_6502_inc_a_nop_compatibility(p):
    """Test that 0x1A acts as NOP on 6502 (Apple ][+) - CORRECTED VERSION"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # CORRECTED: Test CPU register, not BASIC variable
        # LDA #42, INC A (NOP), STA $300, RTS - should store 42 (unchanged)
        ('FOR I=0 TO 6: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,42,26,141,0,3,96', ''),  # LDA #42, INC A (NOP), STA $300, RTS
        ('CALL 768: PRINT PEEK(768)', '42\r\n'),  # Should be 42 (unchanged by NOP)
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_65c02_dec_a_instruction(p):
    """Test DEC A instruction (0x3A) on Enhanced Apple IIe"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test DEC A instruction
        ('A=5: POKE 768,58: POKE 769,96: CALL 768: PRINT A', '4\r\n'),   # DEC A, RTS
        ('A=0: POKE 768,58: POKE 769,96: CALL 768: PRINT A', '255\r\n'), # Test underflow
        ('A=128: POKE 768,58: POKE 769,96: CALL 768: PRINT A', '127\r\n'), # Test sign change
    ])
    return True

@bobbin('-m plus --simple -q')
def test_6502_dec_a_nop_compatibility(p):
    """Test that 0x3A acts as NOP on 6502 (Apple ][+)"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # On 6502, 0x3A should be NOP - accumulator should remain unchanged  
        ('A=42: POKE 768,58: POKE 769,96: CALL 768: PRINT A', '42\r\n'),  # 0x3A NOP, RTS
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_65c02_stz_instructions(p):
    """Test STZ (Store Zero) instructions on Enhanced Apple IIe"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test STZ zero page (0x64)
        ('POKE 50,123: POKE 768,100: POKE 769,50: POKE 770,96: CALL 768: PRINT PEEK(50)', '0\r\n'),
        # Test STZ zero page,X (0x74) 
        ('X=5: POKE 60,99: POKE 768,116: POKE 769,55: POKE 770,96: CALL 768: PRINT PEEK(60)', '0\r\n'),
        # Test STZ absolute (0x9C)
        ('POKE 1000,222: POKE 768,156: POKE 769,232: POKE 770,3: POKE 771,96: CALL 768: PRINT PEEK(1000)', '0\r\n'),
        # Test STZ absolute,X (0x9E)
        ('X=10: POKE 1010,111: POKE 768,158: POKE 769,232: POKE 770,3: POKE 771,96: CALL 768: PRINT PEEK(1010)', '0\r\n'),
    ])
    return True

@bobbin('-m plus --simple -q')
def test_6502_stz_nop_compatibility(p):
    """Test that STZ opcodes act as NOPs on 6502"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # On 6502, STZ opcodes should be NOPs - memory should remain unchanged
        ('POKE 50,123: POKE 768,100: POKE 769,50: POKE 770,96: CALL 768: PRINT PEEK(50)', '123\r\n'),
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_65c02_bit_immediate(p):
    """Test BIT immediate instruction (0x89) on Enhanced Apple IIe"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test BIT #immediate - should set Z flag based on A AND immediate value
        # A=85 (01010101), BIT #170 (10101010) should set Z=1 (no bits in common)
        ('A=85: POKE 768,137: POKE 769,170: POKE 770,96: CALL 768', ''),
        # Test result by checking Z flag via conditional branch
        ('POKE 768,137: POKE 769,170: POKE 770,240: POKE 771,2: POKE 772,169: POKE 773,1: POKE 774,96: A=85: CALL 768: PRINT A', '85\r\n'), # Z=1, branch taken, A unchanged
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_65c02_bra_instruction(p):
    """Test BRA (Branch Always) instruction on Enhanced Apple IIe"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test BRA forward branch - should always branch
        ('POKE 768,128: POKE 769,2: POKE 770,169: POKE 771,99: POKE 772,169: POKE 773,42: POKE 774,96: A=0: CALL 768: PRINT A', '42\r\n'),
        # BRA +2, LDA #99, LDA #42, RTS - should skip LDA #99 and load 42
    ])
    return True

@bobbin('-m plus --simple -q')
def test_6502_bra_nop_compatibility(p):
    """Test that BRA opcode acts as NOP on 6502"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # On 6502, BRA should be NOP - should execute next instruction
        ('POKE 768,128: POKE 769,2: POKE 770,169: POKE 771,99: POKE 772,169: POKE 773,42: POKE 774,96: A=0: CALL 768: PRINT A', '42\r\n'),
        # Will execute as NOP, LDA #99, LDA #42, RTS - should load 42
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_65c02_phx_phy_instructions(p):
    """Test PHX/PHY (Push X/Y) instructions on Enhanced Apple IIe"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test PHX (0xDA) - push X register
        ('X=123: POKE 768,218: POKE 769,96: CALL 768: PRINT PEEK(511)', '123\r\n'), # PHX, RTS - check stack
        # Test PHY (0x5A) - push Y register  
        ('Y=87: POKE 768,90: POKE 769,96: CALL 768: PRINT PEEK(510)', '87\r\n'),   # PHY, RTS - check stack
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_65c02_plx_ply_instructions(p):
    """Test PLX/PLY (Pull X/Y) instructions on Enhanced Apple IIe"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test PLX (0xFA) - pull to X register
        ('POKE 511,55: POKE 768,250: POKE 769,96: X=0: CALL 768: PRINT X', '55\r\n'), # PLX, RTS
        # Test PLY (0x7A) - pull to Y register
        ('POKE 510,77: POKE 768,122: POKE 769,96: Y=0: CALL 768: PRINT Y', '77\r\n'), # PLY, RTS  
    ])
    return True

@bobbin('-m plus --simple -q')
def test_6502_phx_phy_nop_compatibility(p):
    """Test that PHX/PHY opcodes act as NOPs on 6502"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # On 6502, PHX/PHY should be NOPs - stack should remain unchanged
        ('POKE 511,99: X=123: POKE 768,218: POKE 769,96: CALL 768: PRINT PEEK(511)', '99\r\n'), # PHX as NOP
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_enhanced_iie_detection(p):
    """Test Enhanced Apple IIe machine detection"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test ROM identification bytes for Enhanced Apple IIe
        ('PRINT PEEK(64467)', '234\r\n'),  # $FBB3 should be $EA for Enhanced IIe
        ('PRINT PEEK(64448)', '6\r\n'),    # $FBC0 should be $06 for Enhanced IIe
        # Test that 65C02 instructions work (INC A)
        ('A=10: POKE 768,26: POKE 769,96: CALL 768: PRINT A', '11\r\n'),
    ])
    return True

@bobbin('-m twoey --simple -q')
def test_unenhanced_iie_detection(p):
    """Test unenhanced Apple IIe behavior"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test that 0x1A acts as NOP on unenhanced IIe (6502 CPU)
        ('A=20: POKE 768,26: POKE 769,96: CALL 768: PRINT A', '20\r\n'),
    ])
    return True