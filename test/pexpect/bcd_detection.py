#!/usr/bin/python

from common import *

# ProDOS BCD flag test for 65C02 CPU detection
# This is a sophisticated CPU detection method used by ProDOS

@bobbin('-m enhanced --simple -q')
def test_65c02_bcd_flag_detection(p):
    """Test ProDOS BCD flag test for 65C02 detection on Enhanced Apple IIe"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # ProDOS BCD detection sequence:
        # 1. SED (Set Decimal mode)
        # 2. CLD (Clear Decimal mode) - on 65C02 this actually clears it, on 6502 it's ignored
        # 3. LDA #$99, CLC, ADC #$01 
        # 4. On 65C02: BCD is disabled, so $99 + $01 = $9A (binary addition)
        # 5. On 6502: BCD persists despite CLD, so $99 + $01 = $00 (BCD: 99+1=100, keep low byte)
        
        # Assembly sequence: SED, CLD, LDA #$99, CLC, ADC #$01, STA $300, RTS
        # Opcodes: 248, 216, 169, 153, 24, 105, 1, 141, 0, 3, 96
        ('FOR I=0 TO 10: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 248,216,169,153,24,105,1,141,0,3,96', ''),
        ('CALL 768: PRINT PEEK(768)', '154\\r\\n'),  # Should be 154 ($9A) on 65C02
        
        # Verification: 153 + 1 = 154 in binary (65C02 behavior)
        ('PRINT 153 + 1', '154\\r\\n'),  # Confirm expected result
    ])
    return True

@bobbin('-m plus --simple -q')
def test_6502_bcd_flag_detection(p):
    """Test ProDOS BCD flag test on 6502 (Apple ][+)"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Same BCD detection sequence on 6502
        # On 6502: CLD doesn't actually clear decimal mode in this context
        # So $99 + $01 in BCD mode = $00 (99 + 1 = 100 BCD, low byte = 0)
        
        ('FOR I=0 TO 10: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 248,216,169,153,24,105,1,141,0,3,96', ''),
        ('CALL 768: PRINT PEEK(768)', '0\\r\\n'),     # Should be 0 on 6502 (BCD result)
    ])
    return True

@bobbin('-m twoey --simple -q')
def test_unenhanced_iie_bcd_flag_detection(p):
    """Test ProDOS BCD flag test on unenhanced Apple IIe (6502)"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Unenhanced Apple IIe has 6502 CPU, should behave like Apple ][+
        ('FOR I=0 TO 10: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 248,216,169,153,24,105,1,141,0,3,96', ''),
        ('CALL 768: PRINT PEEK(768)', '0\\r\\n'),     # Should be 0 on 6502
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_65c02_bcd_comprehensive_detection(p):
    """Test comprehensive 65C02 detection using multiple BCD test values"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test multiple BCD values to ensure consistent 65C02 behavior
        
        # Test 1: $99 + $01 = $9A (154) on 65C02, $00 (0) on 6502
        ('FOR I=0 TO 10: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 248,216,169,153,24,105,1,141,0,3,96', ''),
        ('CALL 768: T1 = PEEK(768)', ''),
        
        # Test 2: $88 + $01 = $89 (137) on 65C02, $89 (137) on 6502 (same result)
        ('FOR I=0 TO 10: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 248,216,169,136,24,105,1,141,1,3,96', ''),
        ('CALL 768: T2 = PEEK(769)', ''),
        
        # Test 3: $55 + $44 = $99 (153) on 65C02, $99 (153) on 6502 (same result)  
        ('FOR I=0 TO 10: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 248,216,169,85,24,105,68,141,2,3,96', ''),
        ('CALL 768: T3 = PEEK(770)', ''),
        
        # 65C02 should give: T1=154, T2=137, T3=153
        ('PRINT T1: PRINT T2: PRINT T3', '154\\r\\n137\\r\\n153\\r\\n'),
        
        # Detection logic: if T1=154 then 65C02, else 6502
        ('IF T1 = 154 THEN PRINT "65C02" ELSE PRINT "6502"', '65C02\\r\\n'),
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_bcd_and_rom_combined_detection(p):
    """Test combined BCD and ROM detection for Enhanced Apple IIe"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Combined detection: ROM identification + BCD CPU test
        
        # Check ROM bytes first
        ('ROM_FBB3 = PEEK(64435)', ''),    # Should be 6
        ('ROM_FBC0 = PEEK(64448)', ''),    # Should be 224 ($E0)
        
        # BCD CPU test
        ('FOR I=0 TO 10: read D: POKE 768+I,D: NEXT', ''),
        ('DATA 248,216,169,153,24,105,1,141,0,3,96', ''),
        ('CALL 768: BCD_RESULT = PEEK(768)', ''),
        
        # Verification
        ('PRINT ROM_FBB3: PRINT ROM_FBC0: PRINT BCD_RESULT', '6\\r\\n224\\r\\n154\\r\\n'),
        
        # Combined logic for Enhanced Apple IIe detection
        ('ROM_OK = (ROM_FBB3 = 6) AND (ROM_FBC0 = 224)', ''),
        ('CPU_OK = (BCD_RESULT = 154)', ''),
        ('IF ROM_OK AND CPU_OK THEN PRINT "ENHANCED_IIE" ELSE PRINT "OTHER"', 'ENHANCED_IIE\\r\\n'),
    ])
    return True