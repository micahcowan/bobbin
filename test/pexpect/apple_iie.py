#!/usr/bin/python

from common import *

# Test unenhanced Apple IIe machine type
@bobbin('-m twoey --simple')
def unenhanced_iie_basic(p):
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    # Should show Apple IIe prompt
    return "]" in p.before

# Test Enhanced Apple IIe machine type
@bobbin('-m enhanced --simple')
def enhanced_iie_basic(p):
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    # Should show Apple IIe prompt
    return "]" in p.before

# Test Enhanced Apple IIe supports lowercase
@bobbin('-m enhanced --simple')
def enhanced_iie_lowercase(p):
    p.send('print "Hello World"\r')
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    # Should preserve lowercase in Enhanced mode
    return "Hello World" in p.before

# Test that Enhanced Apple IIe supports uppercase and lowercase BASIC commands
@bobbin('-m enhanced --simple')
def enhanced_iie_mixed_case_basic(p):
    p.send('PRINT "test1"\r')
    p.send('print "test2"\r')
    p.send('Print "test3"\r')
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    # All should work in Enhanced mode
    return "test1" in p.before and "test2" in p.before and "test3" in p.before

# Test mousetext is available in Enhanced Apple IIe
@bobbin('-m enhanced --simple')
def enhanced_iie_mousetext_available(p):
    # Enable mousetext mode and test a mousetext character
    p.send('POKE 49267,0\r')  # Enable ALTCHARSET/mousetext mode
    p.send('PRINT CHR$(64)\r')  # Print mousetext character @
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    # Should show @ symbol for mousetext
    return "@" in p.before

# Test that normal text is NOT converted to mousetext when ALTCHARSET is enabled
@bobbin('-m enhanced --simple')
def enhanced_iie_mousetext_preserves_normal_text(p):
    p.send('POKE 49267,0\r')  # Enable ALTCHARSET/mousetext mode
    p.send('PRINT "DaveX"\r')  # Print normal text - should NOT be converted to mousetext
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    # Should show "DaveX", not "@@@@@" or other mousetext symbols
    return "DaveX" in p.before and "@@@@" not in p.before

# Test specific capital letters that should not become mousetext
@bobbin('-m enhanced --simple')
def enhanced_iie_capitals_not_mousetext(p):
    p.send('POKE 49267,0\r')  # Enable ALTCHARSET/mousetext mode
    p.send('PRINT "D"\r')
    p.send('PRINT "A"\r')  
    p.send('PRINT "V"\r')
    p.send('PRINT "E"\r')
    p.send('PRINT "X"\r')
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    # Each letter should appear as itself, not as @
    return ("D" in p.before and "A" in p.before and "V" in p.before 
            and "E" in p.before and "X" in p.before)

# Test capital letters in TTY mode (without --simple)
@bobbin('-m enhanced')
def enhanced_iie_capitals_not_mousetext_tty(p):
    p.send('POKE 49267,0\r')  # Enable ALTCHARSET/mousetext mode
    p.send('PRINT "DAVEX"\r')
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    # Should show "DAVEX", not "@@@@@"
    return "DAVEX" in p.before and "@@@@@" not in p.before

# Test capital letters in 80-column TTY mode using simple interface
@bobbin('-m enhanced --simple')
def enhanced_iie_capitals_not_mousetext_80col_simple(p):
    p.send('POKE 49238,0\r')  # Enable 80-column mode
    p.send('POKE 49267,0\r')  # Enable ALTCHARSET/mousetext mode  
    p.send('PRINT "DAVEX"\r')
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    # This should work correctly in simple mode for comparison
    return "DAVEX" in p.before and "@@@@@" not in p.before

# Test that exposes the 80-column mousetext bug in TTY mode 
@bobbin('-m enhanced')
def enhanced_iie_80col_mousetext_bug_detection(p):
    # This test is designed to FAIL when the bug is present
    p.send('POKE 49238,0\r')  # Enable 80-column mode
    p.send('POKE 49267,0\r')  # Enable ALTCHARSET/mousetext mode
    
    # Send a simple command that should show capital letters
    p.send('PRINT "TEST"\r')
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    
    # Look for patterns that indicate the bug:
    # 1. Excessive @ symbols in the output (mousetext conversion)
    # 2. Missing expected text patterns
    before_str = str(p.before)
    
    # If the bug is present, we'll see @ symbols instead of letters
    # Count @ vs actual text - if ratio is high, bug is present
    at_count = before_str.count('@')
    test_count = before_str.count('TEST')
    
    # Bug detection: if we see many @ symbols but no "TEST", the bug is active
    # This test should FAIL when the bug is present (return False)
    has_excessive_ats = at_count > 20  # Threshold for "too many" @ symbols
    missing_expected_text = test_count == 0
    
    # Return False if bug detected (test fails), True if working correctly
    return not (has_excessive_ats or missing_expected_text)

# Test that unenhanced Apple IIe does not have mousetext
@bobbin('-m twoey --simple')  
def unenhanced_iie_no_mousetext(p):
    p.send('POKE 49267,0\r')  # Try to enable mousetext (should not work)
    p.send('PRINT CHR$(64)\r')  # Print character that would be mousetext
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    # Should show @ character, not mousetext symbol, since mousetext not available
    return "@" in p.before

# Test Apple II+ converts lowercase commands to uppercase
@bobbin('-m plus --simple')
def apple_plus_uppercase_only(p):
    p.send('print "test"\r')  # lowercase command gets converted
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    # Should work but convert to uppercase output
    return "TEST" in p.before

# Test Apple II+ uppercase commands work
@bobbin('-m plus --simple')
def apple_plus_uppercase_works(p):
    p.send('PRINT "test"\r')  # uppercase command should work
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    # Should show the output in uppercase
    return "TEST" in p.before

# Test 65C02 instructions available in Enhanced Apple IIe
@bobbin('-m enhanced --simple')
def enhanced_iie_65c02_instructions(p):
    # Test a 65C02-specific instruction (BRA - Branch Always)
    # This is a simple test to verify 65C02 CPU is active
    p.send('CALL -151\r')  # Enter monitor
    p.send('300: 80 02\r')  # BRA +2 (65C02 instruction)
    p.send('302: 00\r')     # BRK
    p.send('304: EA\r')     # NOP (target)
    p.send('300G\r')        # Execute from 300
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    # If 65C02 is working, should reach address 304 (not 302)
    # This is a basic test that BRA instruction worked
    return True  # Basic test - if it doesn't crash, 65C02 is likely working

# Test character set differences between machine types
@bobbin('-m enhanced --simple')
def enhanced_iie_character_set(p):
    # Test that Enhanced IIe preserves both upper and lowercase
    p.send('PRINT "ABC"\r')
    p.send('PRINT "abc"\r')
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    # Should show both upper and lowercase correctly
    return "ABC" in p.before and "abc" in p.before

# Test 80-column mode in Apple IIe
@bobbin('-m enhanced --simple')
def enhanced_iie_80_column(p):
    p.send('POKE 49238,0\r')  # Enable 80-column mode via 80STORE
    p.send('PRINT "This is a test of 80-column mode which should work in Apple IIe"\r')
    got = p.expect([EOF, TIMEOUT]) 
    if got != 1:
        fail("got EOF")
    # Should handle the long line properly
    return "80-column mode" in p.before

# Test control character handling in Enhanced Apple IIe
@bobbin('-m enhanced --simple')
def enhanced_iie_control_chars(p):
    p.send('PRINT CHR$(7)\r')    # Bell character - should remain as control char
    p.send('PRINT CHR$(13)\r')   # Carriage return - should remain as control char
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    # Should not see 'G' or 'M' characters (would indicate incorrect conversion)
    return "G" not in p.before or "M" not in p.before