#!/usr/bin/python

from common import *

# Test MouseText functionality on Enhanced Apple IIe

@bobbin('-m enhanced --simple -q')
def test_mousetext_activation_basic(p):
    """Test basic MouseText activation with ALTCHAR switch"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # With ALTCHAR disabled - should display normal characters
        ('POKE 49168,0', ''),  # Disable ALTCHAR
        ('PRINT CHR$(65)', 'A\r\n'),    # Should display 'A'
        ('PRINT CHR$(64)', '@\r\n'),    # Should display '@'
        # With ALTCHAR enabled - should display MouseText as '@'
        ('POKE 49167,0', ''),  # Enable ALTCHAR
        ('PRINT CHR$(65)', '@\r\n'),    # Should display '@' (MouseText)
        ('PRINT CHR$(64)', '@\r\n'),    # Should display '@' (MouseText)
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_mousetext_range_40_5f(p):
    """Test entire MouseText range $40-$5F displays as '@'"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        ('POKE 49167,0', ''),  # Enable ALTCHAR
        # Test @ symbol (0x40)
        ('PRINT CHR$(64)', '@\r\n'),
        # Test A-Z range (0x41-0x5A) - all should be MouseText '@'
        ('FOR I=65 TO 70: PRINT CHR$(I);: NEXT: PRINT', '@@@@@@\r\n'),
        ('FOR I=71 TO 76: PRINT CHR$(I);: NEXT: PRINT', '@@@@@@\r\n'),
        ('FOR I=77 TO 82: PRINT CHR$(I);: NEXT: PRINT', '@@@@@@\r\n'),
        ('FOR I=83 TO 88: PRINT CHR$(I);: NEXT: PRINT', '@@@@@@\r\n'),
        ('FOR I=89 TO 90: PRINT CHR$(I);: NEXT: PRINT', '@@\r\n'),
        # Test [\]^_ symbols (0x5B-0x5F) - all should be MouseText '@'
        ('FOR I=91 TO 95: PRINT CHR$(I);: NEXT: PRINT', '@@@@@\r\n'),
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_mousetext_mixed_with_lowercase(p):
    """Test MouseText mixed with lowercase characters"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        ('POKE 49167,0', ''),  # Enable ALTCHAR
        # Lowercase characters should remain as lowercase
        ('PRINT "hello"', 'hello\r\n'),
        # Uppercase characters should become MouseText
        ('PRINT "HELLO"', '@@@@@\r\n'),
        # Mixed case should show both
        ('PRINT "Hello"', '@ello\r\n'),
        ('PRINT "HeLLo"', '@e@@o\r\n'),
        # Test with symbols
        ('PRINT "hello@WORLD"', 'hello@@@@@@\r\n'),
    ])
    return True

@bobbin('-m enhanced --simple -q')  
def test_mousetext_normal_print_statements(p):
    """Test that normal PRINT statements work with MouseText enabled"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        ('POKE 49167,0', ''),  # Enable ALTCHAR
        # Normal text should work, with uppercase becoming MouseText
        ('PRINT "Programming is fun!"', '@rogramming is fun!\r\n'),
        ('PRINT "apple computer"', 'apple computer\r\n'),
        ('PRINT "APPLE COMPUTER"', '@@@@@ @@@@@@@@@\r\n'),
        # Numbers and punctuation should work normally
        ('PRINT "12345!@#$%"', '12345!@#$%\r\n'),
        ('PRINT 42', '42\r\n'),
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_mousetext_chr_function(p):
    """Test CHR$ function with MouseText enabled"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        ('POKE 49167,0', ''),  # Enable ALTCHAR
        # Test specific MouseText character codes
        ('PRINT CHR$(64)', '@\r\n'),    # @ symbol -> MouseText
        ('PRINT CHR$(65)', '@\r\n'),    # A -> MouseText  
        ('PRINT CHR$(66)', '@\r\n'),    # B -> MouseText
        ('PRINT CHR$(72)', '@\r\n'),    # H -> MouseText (hourglass)
        ('PRINT CHR$(84)', '@\r\n'),    # T -> MouseText (timer)
        ('PRINT CHR$(91)', '@\r\n'),    # [ -> MouseText
        ('PRINT CHR$(95)', '@\r\n'),    # _ -> MouseText
        # Test non-MouseText characters remain normal
        ('PRINT CHR$(97)', 'a\r\n'),    # lowercase a
        ('PRINT CHR$(32)', ' \r\n'),    # space
        ('PRINT CHR$(48)', '0\r\n'),    # zero
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_mousetext_toggle_behavior(p):
    """Test toggling MouseText on and off"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Start with ALTCHAR off
        ('POKE 49168,0', ''),  # Disable ALTCHAR
        ('PRINT "HELLO"', 'HELLO\r\n'),
        # Enable MouseText
        ('POKE 49167,0', ''),  # Enable ALTCHAR
        ('PRINT "HELLO"', '@@@@@\r\n'),
        # Disable MouseText
        ('POKE 49168,0', ''),  # Disable ALTCHAR
        ('PRINT "HELLO"', 'HELLO\r\n'),
        # Enable again
        ('POKE 49167,0', ''),  # Enable ALTCHAR
        ('PRINT "HELLO"', '@@@@@\r\n'),
    ])
    return True

@bobbin('-m plus --simple -q')
def test_mousetext_not_available_apple_plus(p):
    """Test MouseText is not available on Apple ][+"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # ALTCHAR switch should not affect Apple ][+ behavior
        ('POKE 49167,0', ''),  # Try to enable ALTCHAR
        ('PRINT "HELLO"', 'HELLO\r\n'),  # Should remain uppercase
        ('PRINT CHR$(65)', 'A\r\n'),     # Should display normal 'A'
        ('PRINT CHR$(72)', 'H\r\n'),     # Should display normal 'H'
        # Apple ][+ doesn't have lowercase either
        ('PRINT "hello"', 'HELLO\r\n'),  # Should convert to uppercase
    ])
    return True

@bobbin('-m twoey --simple -q')
def test_mousetext_not_available_unenhanced_iie(p):
    """Test MouseText is not available on unenhanced Apple IIe"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        # Unenhanced IIe has lowercase but not MouseText
        ('POKE 49167,0', ''),  # Enable ALTCHAR
        ('PRINT "hello"', 'hello\r\n'),  # Should show lowercase
        ('PRINT "HELLO"', 'HELLO\r\n'),  # Should show normal uppercase, not MouseText
        ('PRINT CHR$(65)', 'A\r\n'),     # Should display normal 'A', not MouseText
        ('PRINT CHR$(72)', 'H\r\n'),     # Should display normal 'H', not MouseText
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_mousetext_boundary_characters(p):
    """Test characters at the boundaries of MouseText range"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        ('POKE 49167,0', ''),  # Enable ALTCHAR
        # Characters just before MouseText range (0x3F)
        ('PRINT CHR$(63)', '?\r\n'),     # Should display normal '?'
        # First MouseText character (0x40)
        ('PRINT CHR$(64)', '@\r\n'),     # Should display MouseText '@'
        # Last MouseText character (0x5F)
        ('PRINT CHR$(95)', '@\r\n'),     # Should display MouseText '@'
        # Characters just after MouseText range (0x60)
        ('PRINT CHR$(96)', '`\r\n'),     # Should display normal '`'
        # Test control characters are not affected
        ('PRINT CHR$(13)', '\r\r\n'),    # CR should work normally
        ('PRINT CHR$(32)', ' \r\n'),     # Space should work normally
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_mousetext_with_numbers_and_symbols(p):
    """Test MouseText behavior with numbers and symbols"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        ('POKE 49167,0', ''),  # Enable ALTCHAR
        # Numbers should not be affected by MouseText
        ('PRINT "12345"', '12345\r\n'),
        # Common symbols outside MouseText range
        ('PRINT "!#$%&*()+"', '!#$%&*()+\r\n'),
        # Symbols in MouseText range should become MouseText
        ('PRINT "@[\\]^_"', '@@@@@@\r\n'),
        # Mixed numbers, symbols, and MouseText
        ('PRINT "123ABC!@#"', '123@@@!@#\r\n'),
    ])
    return True

@bobbin('-m enhanced --simple -q')
def test_mousetext_consistency_check(p):
    """Test MouseText behavior is consistent across multiple calls"""
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        ('POKE 49167,0', ''),  # Enable ALTCHAR
        # Test same characters multiple times
        ('PRINT CHR$(65); CHR$(65); CHR$(65)', '@@@ \r\n'),
        ('PRINT "AAA"', '@@@\r\n'),
        ('FOR I=1 TO 3: PRINT CHR$(65);: NEXT: PRINT', '@@@\r\n'),
        # Test mixed repeated patterns
        ('PRINT "AaAaAa"', '@a@a@a\r\n'),
        ('PRINT "HelloHELLO"', '@ello@@@@@\r\n'),
    ])
    return True