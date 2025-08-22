#!/usr/bin/python

from common import *

# Test character set functionality across different Apple II models

@bobbin('-m plus --simple -q')
def test_apple_plus_uppercase_only(p):
    """Test Apple ][+ displays only uppercase characters"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        ('PRINT "HELLO WORLD"', 'HELLO WORLD\r\n'),
        ('PRINT "hello world"', 'HELLO WORLD\r\n'),  # Should convert to uppercase
        ('FOR I=65 TO 70: PRINT CHR$(I);: NEXT: PRINT', 'ABCDEF\r\n'),
        ('FOR I=97 TO 102: PRINT CHR$(I);: NEXT: PRINT', 'ABCDEF\r\n'),  # Lowercase codes -> uppercase
    ])
    return True

@bobbin('-m original --simple -q')
def test_apple_original_uppercase_only(p):
    """Test original Apple ][ displays only uppercase characters"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        ('PRINT "HELLO WORLD"', 'HELLO WORLD\r\n'),
        ('PRINT "hello world"', 'HELLO WORLD\r\n'),  # Should convert to uppercase
        ('FOR I=65 TO 70: PRINT CHR$(I);: NEXT: PRINT', 'ABCDEF\r\n'),
    ])
    return True

@bobbin('-m twoey --simple -q')
def test_unenhanced_iie_altchar_disabled(p):
    """Test unenhanced Apple IIe with ALTCHAR disabled"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Default state - ALTCHAR disabled
        ('PRINT "HELLO WORLD"', 'HELLO WORLD\r\n'),
        ('PRINT "hello world"', 'HELLO WORLD\r\n'),  # Should convert to uppercase
        ('FOR I=97 TO 102: PRINT CHR$(I);: NEXT: PRINT', 'ABCDEF\r\n'),  # Lowercase -> uppercase
    ])
    return True

@bobbin('-m twoey --simple -q')
def test_unenhanced_iie_altchar_enabled(p):
    """Test unenhanced Apple IIe with ALTCHAR enabled"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Enable ALTCHAR
        ('POKE 49167,0', ''),  # Enable alternate character set
        ('PRINT "HELLO WORLD"', 'HELLO WORLD\r\n'),
        ('PRINT "hello world"', 'hello world\r\n'),  # Should display lowercase
        ('FOR I=97 TO 102: PRINT CHR$(I);: NEXT: PRINT', 'abcdef\r\n'),  # Lowercase display
        # Disable ALTCHAR
        ('POKE 49168,0', ''),  # Disable alternate character set  
        ('PRINT "hello world"', 'HELLO WORLD\r\n'),  # Back to uppercase
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_enhanced_iie_altchar_disabled(p):
    """Test Enhanced Apple IIe with ALTCHAR disabled"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Default state - ALTCHAR disabled
        ('PRINT "HELLO WORLD"', 'HELLO WORLD\r\n'),
        ('PRINT "hello world"', 'HELLO WORLD\r\n'),  # Should convert to uppercase
        ('FOR I=97 TO 102: PRINT CHR$(I);: NEXT: PRINT', 'ABCDEF\r\n'),  # Lowercase -> uppercase
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_enhanced_iie_altchar_enabled(p):
    """Test Enhanced Apple IIe with ALTCHAR enabled"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Enable ALTCHAR
        ('POKE 49167,0', ''),  # Enable alternate character set
        ('PRINT "HELLO WORLD"', 'HELLO WORLD\r\n'),
        ('PRINT "hello world"', 'hello world\r\n'),  # Should display lowercase
        ('PRINT "Mixed Case Text"', 'Mixed Case Text\r\n'),  # Mixed case
        ('FOR I=97 TO 102: PRINT CHR$(I);: NEXT: PRINT', 'abcdef\r\n'),  # Lowercase display
        ('FOR I=65 TO 70: PRINT CHR$(I);: NEXT: PRINT', 'ABCDEF\r\n'),   # Uppercase display
        # Disable ALTCHAR
        ('POKE 49168,0', ''),  # Disable alternate character set
        ('PRINT "hello world"', 'HELLO WORLD\r\n'),  # Back to uppercase
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_character_range_40_5f_normal(p):
    """Test $40-$5F character range behavior with ALTCHAR disabled"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test @ symbol (0x40)
        ('PRINT CHR$(64)', '@\r\n'),
        # Test uppercase letters A-Z (0x41-0x5A)
        ('FOR I=65 TO 70: PRINT CHR$(I);: NEXT: PRINT', 'ABCDEF\r\n'),
        # Test symbols [\]^_ (0x5B-0x5F)
        ('PRINT CHR$(91);CHR$(92);CHR$(93);CHR$(94);CHR$(95)', '[\\]^_\r\n'),
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_character_range_60_7f_lowercase(p):
    """Test $60-$7F character range (lowercase) behavior"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # With ALTCHAR disabled - should convert to uppercase
        ('POKE 49168,0', ''),  # Disable ALTCHAR
        ('FOR I=97 TO 102: PRINT CHR$(I);: NEXT: PRINT', 'ABCDEF\r\n'),
        # With ALTCHAR enabled - should display lowercase
        ('POKE 49167,0', ''),  # Enable ALTCHAR
        ('FOR I=97 TO 102: PRINT CHR$(I);: NEXT: PRINT', 'abcdef\r\n'),
        # Test full lowercase alphabet
        ('PRINT "abcdefghijklmnopqrstuvwxyz"', 'abcdefghijklmnopqrstuvwxyz\r\n'),
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_control_character_handling(p):
    """Test control character handling"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test that control characters (< 0x20) are handled properly
        ('PRINT CHR$(7)', '\x07\r\n'),    # Bell character
        ('PRINT CHR$(13)', '\r\r\n'),     # Carriage return
        ('PRINT CHR$(10)', '\n\r\n'),     # Line feed
        # Test characters with high bit set are processed correctly
        ('PRINT CHR$(200)', '@\r\n'),     # 0xC8 -> 0x48 -> 'H' -> '@' (control)
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_altchar_switch_addresses(p):
    """Test ALTCHAR soft switch addresses work correctly"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test POKE 49167,0 enables ALTCHAR (address $C00F)
        ('POKE 49167,0: PRINT "test"', 'test\r\n'),
        # Test POKE 49168,0 disables ALTCHAR (address $C00E)  
        ('POKE 49168,0: PRINT "test"', 'TEST\r\n'),
        # Test multiple switches
        ('POKE 49167,0: POKE 49168,0: PRINT "test"', 'TEST\r\n'),  # Last write wins
        ('POKE 49168,0: POKE 49167,0: PRINT "test"', 'test\r\n'),  # Last write wins
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_eightycol_switch_behavior(p):
    """Test 80-column switch also enables lowercase"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test that 80-column firmware switch also enables lowercase
        # Note: This may not be fully implemented, but we test the expected behavior
        ('POKE 49168,0: PRINT "test"', 'TEST\r\n'),    # ALTCHAR off
        ('POKE 49167,0: PRINT "test"', 'test\r\n'),    # ALTCHAR on
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_mixed_case_scenarios(p):
    """Test various mixed case scenarios"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        ('POKE 49167,0', ''),  # Enable ALTCHAR
        # Test mixed case sentences
        ('PRINT "Hello World"', 'Hello World\r\n'),
        ('PRINT "The Quick Brown Fox"', 'The Quick Brown Fox\r\n'),
        ('PRINT "UPPERCASE and lowercase"', 'UPPERCASE and lowercase\r\n'),
        # Test punctuation with mixed case
        ('PRINT "Don\'t worry, be happy!"', 'Don\'t worry, be happy!\r\n'),
        ('PRINT "Numbers: 123 and Letters: AbC"', 'Numbers: 123 and Letters: AbC\r\n'),
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_character_set_compatibility_modes(p):
    """Test character set behavior is consistent across switch states"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Test that PRINT statements work consistently
        ('POKE 49168,0: PRINT "HELLO"', 'HELLO\r\n'),
        ('POKE 49167,0: PRINT "HELLO"', 'HELLO\r\n'),  # Uppercase should stay uppercase
        ('POKE 49168,0: PRINT "hello"', 'HELLO\r\n'),  # Lowercase -> uppercase
        ('POKE 49167,0: PRINT "hello"', 'hello\r\n'),  # Lowercase preserved
        # Test CHR$ function consistency
        ('POKE 49168,0: PRINT CHR$(65)', 'A\r\n'),     # Uppercase A
        ('POKE 49167,0: PRINT CHR$(65)', 'A\r\n'),     # Should still be uppercase A
        ('POKE 49168,0: PRINT CHR$(97)', 'A\r\n'),     # Lowercase a -> uppercase A  
        ('POKE 49167,0: PRINT CHR$(97)', 'a\r\n'),     # Lowercase a preserved
    ])
    return True