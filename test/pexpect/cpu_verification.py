#!/usr/bin/python

from common import *

# Proper 65C02 CPU instruction verification tests
# These tests verify actual CPU register and memory changes, not BASIC variables

@bobbin('-m enhanced --simple -q')
def test_65c02_inc_a_cpu_register(p):
    """Test INC A instruction actually modifies CPU accumulator"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test sequence: LDA #value, INC A, STA $300, check memory
        # Assembly: LDA #$42 (169,66), INC A (26), STA $300 (141,0,3), RTS (96)
        ('POKE 768,169: POKE 769,66: POKE 770,26: POKE 771,141: POKE 772,0: POKE 773,3: POKE 774,96', ''),
        ('CALL 768: PRINT PEEK(768)', '169\r\n'),  # Verify POKE worked
        ('CALL 768: PRINT PEEK(773)', '3\r\n'),    # Verify assembly sequence 
        ('CALL 768: PRINT PEEK(774)', '96\r\n'),   # Verify RTS
        ('CALL 768: PRINT PEEK(768)', '169\r\n'),  # Execute and verify memory result
        # The actual test: LDA #66, INC A (66->67), STA $300
        ('POKE 768,169: POKE 769,66: POKE 770,26: POKE 771,141: POKE 772,0: POKE 773,3: POKE 774,96: CALL 768: PRINT PEEK(768)', '67\r\n'),
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_65c02_inc_a_memory_verification(p):
    """Test INC A using memory to verify CPU state changes"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Clear test memory location
        ('POKE 768,0', ''),
        # Test INC A with value 5: LDA #5, INC A, STA $300
        # Opcodes: 169,5,26,141,0,3,96 (LDA #5, INC A, STA $300, RTS)
        ('FOR I=0 TO 6: READ D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,5,26,141,0,3,96', ''),
        ('CALL 768: PRINT PEEK(768)', '6\r\n'),  # Should be 6 (5+1)
        
        # Test INC A with value 255 (wraparound): LDA #255, INC A, STA $301  
        ('FOR I=0 TO 6: READ D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,255,26,141,1,3,96', ''),
        ('CALL 768: PRINT PEEK(769)', '0\r\n'),  # Should be 0 (255+1 wraps)
        
        # Test INC A with value 127 (sign change): LDA #127, INC A, STA $302
        ('FOR I=0 TO 6: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,127,26,141,2,3,96', ''),
        ('CALL 768: PRINT PEEK(770)', '128\r\n'), # Should be 128 (127+1)
    ])
    return True

@bobbin('-m plus --simple -q')
def test_6502_inc_a_nop_memory_verification(p):
    """Test that INC A acts as NOP on 6502 using memory verification"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test that INC A is NOP on 6502: LDA #42, INC A (NOP), STA $300
        ('FOR I=0 TO 6: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,42,26,141,0,3,96', ''),
        ('CALL 768: PRINT PEEK(768)', '42\r\n'),  # Should stay 42 (NOP)
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_65c02_dec_a_cpu_register(p):
    """Test DEC A instruction (0x3A) modifies CPU accumulator"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test DEC A with value 10: LDA #10, DEC A, STA $300
        # Opcodes: 169,10,58,141,0,3,96 (LDA #10, DEC A, STA $300, RTS)
        ('FOR I=0 TO 6: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,10,58,141,0,3,96', ''),
        ('CALL 768: PRINT PEEK(768)', '9\r\n'),   # Should be 9 (10-1)
        
        # Test DEC A with value 0 (underflow): LDA #0, DEC A, STA $301
        ('FOR I=0 TO 6: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,0,58,141,1,3,96', ''),
        ('CALL 768: PRINT PEEK(769)', '255\r\n'), # Should be 255 (0-1 wraps)
        
        # Test DEC A with value 128 (sign change): LDA #128, DEC A, STA $302
        ('FOR I=0 TO 6: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,128,58,141,2,3,96', ''),
        ('CALL 768: PRINT PEEK(770)', '127\r\n'), # Should be 127 (128-1)
    ])
    return True

@bobbin('-m plus --simple -q')
def test_6502_dec_a_nop_memory_verification(p):
    """Test that DEC A acts as NOP on 6502"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test that DEC A is NOP on 6502: LDA #42, DEC A (NOP), STA $300
        ('FOR I=0 TO 6: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,42,58,141,0,3,96', ''),
        ('CALL 768: PRINT PEEK(768)', '42\r\n'),  # Should stay 42 (NOP)
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_65c02_stz_zero_page(p):
    """Test STZ zero page instruction (0x64)"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Set up test: put non-zero value in zero page location $50
        ('POKE 80,123', ''),  # $50 = 80 decimal
        ('PRINT PEEK(80)', '123\r\n'),  # Verify setup
        
        # Test STZ $50: STZ $50, RTS
        # Opcodes: 100,80,96 (STZ $50, RTS)
        ('POKE 768,100: POKE 769,80: POKE 770,96', ''),
        ('CALL 768: PRINT PEEK(80)', '0\r\n'),  # Should be 0
    ])
    return True

@bobbin('-m enhanced --simple -q')  
def test_65c02_stz_absolute(p):
    """Test STZ absolute instruction (0x9C)"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Set up test: put non-zero value in absolute location $400
        ('POKE 1024,99', ''),  # $400 = 1024 decimal  
        ('PRINT PEEK(1024)', '99\r\n'),  # Verify setup
        
        # Test STZ $400: STZ $400, RTS
        # Opcodes: 156,0,4,96 (STZ $400, RTS) - little endian: 0,4 = $400
        ('POKE 768,156: POKE 769,0: POKE 770,4: POKE 771,96', ''),
        ('CALL 768: PRINT PEEK(1024)', '0\r\n'),  # Should be 0
    ])
    return True

@bobbin('-m plus --simple -q')
def test_6502_stz_nop_compatibility(p):
    """Test that STZ instructions act as NOPs on 6502"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test that STZ is NOP on 6502 - memory should remain unchanged
        ('POKE 80,123', ''),  # Set test value
        ('POKE 768,100: POKE 769,80: POKE 770,96', ''),  # STZ $50, RTS
        ('CALL 768: PRINT PEEK(80)', '123\r\n'),  # Should remain 123 (NOP)
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_65c02_bra_instruction(p):
    """Test BRA (Branch Always) instruction (0x80)"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test BRA forward: BRA +4, LDA #99, STA $300, LDA #42, STA $300, RTS
        # Should skip LDA #99 and execute LDA #42
        # Opcodes: 128,4,169,99,141,0,3,169,42,141,0,3,96
        #          BRA +4, LDA #99, STA $300, LDA #42, STA $300, RTS
        ('POKE 768,128: POKE 769,4', ''),        # BRA +4
        ('POKE 770,169: POKE 771,99', ''),       # LDA #99 (should be skipped)  
        ('POKE 772,141: POKE 773,0: POKE 774,3', ''), # STA $300
        ('POKE 775,169: POKE 776,42', ''),       # LDA #42 (should execute)
        ('POKE 777,141: POKE 778,0: POKE 779,3', ''), # STA $300  
        ('POKE 780,96', ''),                     # RTS
        ('CALL 768: PRINT PEEK(768)', '42\r\n'), # Should be 42, not 99
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_65c02_phx_phy_instructions(p):
    """Test PHX/PHY (Push X/Y) instructions"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test PHX: LDX #$55, PHX, LDA $01FF, STA $300, RTS
        # Opcodes: 162,85,218,173,255,1,141,0,3,96
        #          LDX #$55, PHX, LDA $01FF, STA $300, RTS
        ('FOR I=0 TO 9: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 162,85,218,173,255,1,141,0,3,96', ''),
        ('CALL 768: PRINT PEEK(768)', '85\r\n'),  # Should be $55 from stack
        
        # Test PHY: LDY #$AA, PHY, LDA $01FE, STA $301, RTS  
        ('FOR I=0 TO 9: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 164,170,90,173,254,1,141,1,3,96', ''),
        ('CALL 768: PRINT PEEK(769)', '170\r\n'), # Should be $AA from stack
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_65c02_plx_ply_instructions(p):
    """Test PLX/PLY (Pull X/Y) instructions"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Set up stack with test values
        ('POKE 511,77', ''),   # $01FF = 77
        ('POKE 510,88', ''),   # $01FE = 88
        
        # Test PLX: PLX, STX $300, RTS
        # Opcodes: 250,142,0,3,96 (PLX, STX $300, RTS)
        ('FOR I=0 TO 4: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 250,142,0,3,96', ''),
        ('CALL 768: PRINT PEEK(768)', '77\r\n'),  # Should be 77 from stack
        
        # Test PLY: PLY, STY $301, RTS
        # Opcodes: 122,140,1,3,96 (PLY, STY $301, RTS)  
        ('FOR I=0 TO 4: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 122,140,1,3,96', ''),
        ('CALL 768: PRINT PEEK(769)', '88\r\n'),  # Should be 88 from stack
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_65c02_bit_immediate(p):
    """Test BIT immediate instruction (0x89)"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test BIT #immediate: LDA #$55, BIT #$AA, BEQ skip, LDA #1, skip: STA $300, RTS
        # $55 = 01010101, $AA = 10101010, no common bits so Z=1, should branch
        # Opcodes: 169,85,137,170,240,2,169,1,141,0,3,96
        #          LDA #$55, BIT #$AA, BEQ +2, LDA #1, STA $300, RTS
        ('FOR I=0 TO 11: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,85,137,170,240,2,169,1,141,0,3,96', ''),
        ('CALL 768: PRINT PEEK(768)', '85\r\n'),  # Should be $55 (branch taken, skip LDA #1)
        
        # Test with bits in common: LDA #$FF, BIT #$01, BEQ skip, LDA #2, skip: STA $301, RTS  
        # $FF & $01 = $01, Z=0, should not branch
        ('FOR I=0 TO 11: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,255,137,1,240,2,169,2,141,1,3,96', ''),
        ('CALL 768: PRINT PEEK(769)', '2\r\n'),   # Should be 2 (branch not taken)
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_enhanced_iie_cpu_detection(p):
    """Test Enhanced Apple IIe CPU detection using INC A"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # This is how ProDOS detects Enhanced Apple IIe:
        # LDA #0, INC A, CMP #1, BEQ enhanced, LDA #0, BNE done, enhanced: LDA #1, done: STA $300
        # If INC A works, A becomes 1 and branch is taken
        ('FOR I=0 TO 13: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,0,26,201,1,240,4,169,0,208,2,169,1,141,0,3,96', ''),
        ('CALL 768: PRINT PEEK(768)', '1\r\n'),  # Should be 1 (Enhanced IIe detected)
    ])
    return True

@bobbin('-m plus --simple -q')
def test_apple_plus_cpu_detection(p):
    """Test that Apple ][+ is NOT detected as Enhanced IIe"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Same CPU detection sequence on Apple ][+
        ('FOR I=0 TO 13: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,0,26,201,1,240,4,169,0,208,2,169,1,141,0,3,96', ''),
        ('CALL 768: PRINT PEEK(768)', '0\r\n'),  # Should be 0 (NOT Enhanced IIe)
    ])
    return True

@bobbin('-m twoey --simple -q')
def test_unenhanced_iie_cpu_detection(p):
    """Test that unenhanced Apple IIe is NOT detected as Enhanced IIe"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Same CPU detection sequence on unenhanced Apple IIe
        ('FOR I=0 TO 13: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,0,26,201,1,240,4,169,0,208,2,169,1,141,0,3,96', ''),
        ('CALL 768: PRINT PEEK(768)', '0\r\n'),  # Should be 0 (NOT Enhanced IIe)
    ])
    return True