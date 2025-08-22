#!/usr/bin/python

from common import *

# Enhanced Apple IIe detection tests based on Apple Technical Documentation
# These tests verify ROM identification bytes and CPU features

@bobbin('-m enhanced --simple -q')
def test_enhanced_iie_rom_identification_fbb3(p):
    """Test $FBB3 ROM identification byte for Enhanced Apple IIe"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # $FBB3 should be $06 for Apple IIe or later models
        ('PRINT PEEK(64435)', '6\r\n'),  # $FBB3 = 64435 decimal
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_enhanced_iie_rom_identification_fbc0(p):
    """Test $FBC0 ROM identification byte for Enhanced Apple IIe"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # $FBC0 should be $E0 for Enhanced Apple IIe (original IIe has $EA)
        ('PRINT PEEK(64448)', '224\r\n'),  # $FBC0 = 64448 decimal, $E0 = 224 decimal
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_enhanced_iie_rom_identification_fbbf(p):
    """Test $FBBF ROM identification byte for Enhanced Apple IIe"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # $FBBF should be $00 for Enhanced Apple IIe
        ('PRINT PEEK(64447)', '0\r\n'),  # $FBBF = 64447 decimal
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_enhanced_iie_complete_rom_detection(p):
    """Test complete ROM-based Enhanced Apple IIe detection sequence"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Complete Enhanced Apple IIe detection logic
        # IF PEEK($FBB3) = 6 AND PEEK($FBC0) = 224 THEN Enhanced Apple IIe
        ('IF PEEK(64435) = 6 AND PEEK(64448) = 224 THEN PRINT "ENHANCED" ELSE PRINT "NOT_ENHANCED"', 'ENHANCED\r\n'),
    ])
    return True

@bobbin('-m plus --simple -q')
def test_apple_plus_rom_detection(p):
    """Test that Apple ][+ is NOT detected as Enhanced Apple IIe"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Apple ][+ should NOT have the Enhanced Apple IIe identification bytes
        ('IF PEEK(64435) = 6 AND PEEK(64448) = 224 THEN PRINT "ENHANCED" ELSE PRINT "NOT_ENHANCED"', 'NOT_ENHANCED\r\n'),
    ])
    return True

@bobbin('-m twoey --simple -q')
def test_unenhanced_iie_rom_detection(p):
    """Test that unenhanced Apple IIe is NOT detected as Enhanced Apple IIe"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Unenhanced Apple IIe should have $FBB3 = 6 but $FBC0 = $EA (234), not $E0 (224)
        ('PRINT PEEK(64435)', '6\r\n'),    # Should be 6 (Apple IIe or later)
        ('PRINT PEEK(64448)', '234\r\n'),  # Should be 234 ($EA) for unenhanced IIe, not 224 ($E0)
        ('IF PEEK(64435) = 6 AND PEEK(64448) = 224 THEN PRINT "ENHANCED" ELSE PRINT "NOT_ENHANCED"', 'NOT_ENHANCED\r\n'),
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_enhanced_iie_cpu_detection_inc_a(p):
    """Test Enhanced Apple IIe 65C02 CPU detection using INC A instruction"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # ProDOS-style 65C02 CPU detection using INC A
        # LDA #0, INC A, CMP #1, BEQ enhanced
        ('FOR I=0 TO 13: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,0,26,201,1,240,4,169,0,208,2,169,1,141,0,3,96', ''),
        ('CALL 768: PRINT PEEK(768)', '1\r\n'),  # Should be 1 (Enhanced IIe detected)
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_enhanced_iie_cpu_detection_stz(p):
    """Test Enhanced Apple IIe 65C02 CPU detection using STZ instruction"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Alternative 65C02 detection using STZ instruction
        # Set test location to non-zero, execute STZ, check if it became zero
        ('POKE 1024,99', ''),                                    # Set test value
        ('POKE 768,156: POKE 769,0: POKE 770,4: POKE 771,96', ''), # STZ $400, RTS
        ('CALL 768', ''),                                        # Execute STZ
        ('IF PEEK(1024) = 0 THEN PRINT "ENHANCED" ELSE PRINT "NOT_ENHANCED"', 'ENHANCED\r\n'),
    ])
    return True

@bobbin('-m plus --simple -q')
def test_apple_plus_cpu_not_enhanced(p):
    """Test that Apple ][+ 6502 CPU is NOT detected as Enhanced"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Same INC A detection on Apple ][+ (6502 CPU)
        ('FOR I=0 TO 13: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,0,26,201,1,240,4,169,0,208,2,169,1,141,0,3,96', ''),
        ('CALL 768: PRINT PEEK(768)', '0\r\n'),  # Should be 0 (NOT Enhanced IIe)
    ])
    return True

@bobbin('-m twoey --simple -q')
def test_unenhanced_iie_cpu_not_enhanced(p):
    """Test that unenhanced Apple IIe 6502 CPU is NOT detected as Enhanced"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Same INC A detection on unenhanced Apple IIe (6502 CPU)
        ('FOR I=0 TO 13: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,0,26,201,1,240,4,169,0,208,2,169,1,141,0,3,96', ''),
        ('CALL 768: PRINT PEEK(768)', '0\r\n'),  # Should be 0 (NOT Enhanced IIe)
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_enhanced_iie_bcd_flag_detection(p):
    """Test Enhanced Apple IIe 65C02 detection using BCD flag test"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # ProDOS BCD flag test for 65C02 detection
        # SED, CLD, LDA #$99, CLC, ADC #$01, check if A = $9A (BCD) or $00 (binary)
        # On 65C02: BCD disabled by CLD, so $99 + $01 = $9A
        # On 6502: BCD persists, so $99 + $01 = $00 (BCD math: 99 + 1 = 100, keep $00)
        ('FOR I=0 TO 11: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 248,216,169,153,24,105,1,201,154,240,2,169,0,96', ''), # SED,CLD,LDA #$99,CLC,ADC #$01,CMP #$9A,BEQ +2,LDA #0,RTS
        ('CALL 768: IF A = 154 THEN PRINT "ENHANCED" ELSE PRINT "NOT_ENHANCED"', 'ENHANCED\r\n'),
    ])
    return True

@bobbin('-m enhanced --simple -q')  
def test_enhanced_iie_comprehensive_detection(p):
    """Test comprehensive Enhanced Apple IIe detection combining ROM and CPU tests"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Comprehensive detection: ROM bytes + CPU capability
        # Check ROM identification AND 65C02 functionality
        ('ROM_OK = (PEEK(64435) = 6) AND (PEEK(64448) = 224)', ''),
        ('FOR I=0 TO 6: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 169,5,26,141,0,3,96', ''),  # LDA #5, INC A, STA $300, RTS
        ('CALL 768', ''),
        ('CPU_OK = (PEEK(768) = 6)', ''),   # INC A should make 5 become 6
        ('IF ROM_OK AND CPU_OK THEN PRINT "ENHANCED_IIE" ELSE PRINT "OTHER"', 'ENHANCED_IIE\r\n'),
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_enhanced_iie_lowercase_basic_support(p):
    """Test Enhanced Apple IIe lowercase BASIC command support"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Enhanced Apple IIe supports lowercase BASIC commands
        ('print "lowercase works"', 'lowercase works\r\n'),
        ('poke 1000,123', ''),
        ('print peek(1000)', '123\r\n'),
    ])
    return True