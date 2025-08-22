#!/usr/bin/python

from common import *

# Test TTY interface functionality for Enhanced Apple IIe features
# Note: These tests focus on character display in the TTY interface

@bobbin('-m enhanced')
def test_tty_basic_startup(p):
    """Test TTY interface starts up correctly on Enhanced Apple IIe"""
    # TTY interface should start and show the Apple II screen
    p.expect_exact("]")  # Should reach the BASIC prompt
    p.sendline("PRINT \"TTY WORKS\"")
    p.expect_exact("TTY WORKS")
    p.expect_exact("]")
    return True

@bobbin('-m enhanced')
def test_tty_lowercase_support(p):
    """Test lowercase character display in TTY interface"""
    p.expect_exact("]")
    # Enable ALTCHAR for lowercase support
    p.sendline("POKE 49167,0")
    p.expect_exact("]")
    # Test lowercase display
    p.sendline("PRINT \"hello world\"")
    p.expect_exact("hello world")
    p.expect_exact("]")
    # Test mixed case
    p.sendline("PRINT \"Hello World\"")
    p.expect_exact("Hello World") 
    p.expect_exact("]")
    return True

@bobbin('-m enhanced')
def test_tty_uppercase_display(p):
    """Test uppercase character display in TTY interface"""
    p.expect_exact("]")
    # Test uppercase with ALTCHAR disabled (default)
    p.sendline("PRINT \"HELLO WORLD\"")
    p.expect_exact("HELLO WORLD")
    p.expect_exact("]")
    # Test lowercase conversion to uppercase
    p.sendline("PRINT \"hello world\"")
    p.expect_exact("HELLO WORLD")
    p.expect_exact("]")
    return True

@bobbin('-m enhanced')
def test_tty_mousetext_display(p):
    """Test MouseText character display in TTY interface"""
    p.expect_exact("]")
    # Enable ALTCHAR for MouseText
    p.sendline("POKE 49167,0")
    p.expect_exact("]")
    # Test MouseText characters display as '@'
    p.sendline("PRINT CHR$(65)")  # A -> MouseText
    p.expect_exact("@")
    p.expect_exact("]")
    # Test multiple MouseText characters
    p.sendline("PRINT \"HELLO\"")  # Should display as @@@@@ 
    p.expect_exact("@@@@@")
    p.expect_exact("]")
    return True

@bobbin('-m enhanced')
def test_tty_altchar_toggle(p):
    """Test ALTCHAR toggle functionality in TTY interface"""
    p.expect_exact("]")
    # Start with ALTCHAR disabled
    p.sendline("POKE 49168,0")
    p.expect_exact("]")
    p.sendline("PRINT \"Hello\"")
    p.expect_exact("HELLO")  # Should be uppercase
    p.expect_exact("]")
    # Enable ALTCHAR
    p.sendline("POKE 49167,0")
    p.expect_exact("]") 
    p.sendline("PRINT \"Hello\"")
    p.expect_exact("@ello")  # H->@, ello stays lowercase
    p.expect_exact("]")
    # Disable ALTCHAR again
    p.sendline("POKE 49168,0")
    p.expect_exact("]")
    p.sendline("PRINT \"Hello\"")
    p.expect_exact("HELLO")  # Back to uppercase
    p.expect_exact("]")
    return True

@bobbin('-m enhanced')
def test_tty_character_range_consistency(p):
    """Test character range display consistency in TTY interface"""
    p.expect_exact("]")
    p.sendline("POKE 49167,0")  # Enable ALTCHAR
    p.expect_exact("]")
    # Test lowercase range (should display as lowercase)
    p.sendline("FOR I=97 TO 102: PRINT CHR$(I);: NEXT: PRINT")
    p.expect_exact("abcdef")
    p.expect_exact("]")
    # Test uppercase range (should display as MouseText '@')
    p.sendline("FOR I=65 TO 70: PRINT CHR$(I);: NEXT: PRINT")
    p.expect_exact("@@@@@@")
    p.expect_exact("]")
    # Test numbers (should remain unchanged)
    p.sendline("FOR I=48 TO 53: PRINT CHR$(I);: NEXT: PRINT")
    p.expect_exact("012345")
    p.expect_exact("]")
    return True

@bobbin('-m enhanced')
def test_tty_null_character_handling(p):
    """Test null character handling in TTY interface (artifact fix)"""
    p.expect_exact("]")
    # Test that null characters don't cause display artifacts
    p.sendline("POKE 768,0: PRINT CHR$(PEEK(768))")
    # Should not see weird terminal artifacts, just clean display
    p.expect_exact("]")
    # Test other control characters
    p.sendline("PRINT CHR$(1); CHR$(2); CHR$(3)")
    # Should handle control characters gracefully
    p.expect_exact("]")
    return True

@bobbin('-m enhanced') 
def test_tty_mixed_content_display(p):
    """Test mixed content display in TTY interface"""
    p.expect_exact("]")
    p.sendline("POKE 49167,0")  # Enable ALTCHAR
    p.expect_exact("]")
    # Test sentence with mixed case, numbers, and symbols
    p.sendline("PRINT \"The Quick 123 BROWN fox!\"")
    p.expect_exact("@he @uick 123 @@@@@ fox!")
    p.expect_exact("]")
    # Test programming-related content
    p.sendline("PRINT \"FOR I=1 TO 10: PRINT I: NEXT\"")
    p.expect_exact("@@@ @=1 @@ 10: @@@@@ @: @@@@@")
    p.expect_exact("]")
    return True

@bobbin('-m enhanced')
def test_tty_screen_positioning(p):
    """Test that character display doesn't cause screen positioning issues"""
    p.expect_exact("]")
    p.sendline("POKE 49167,0")  # Enable ALTCHAR
    p.expect_exact("]")
    # Test multiple lines of output
    p.sendline("FOR I=1 TO 5: PRINT \"Line\"; I; \"Hello\": NEXT")
    p.expect_exact("@ine 1 @ello")
    p.expect_exact("@ine 2 @ello")  
    p.expect_exact("@ine 3 @ello")
    p.expect_exact("@ine 4 @ello")
    p.expect_exact("@ine 5 @ello")
    p.expect_exact("]")
    return True

@bobbin('-m plus')
def test_tty_compatibility_apple_plus(p):
    """Test TTY interface compatibility with Apple ][+"""
    p.expect_exact("]")
    # Apple ][+ should not support lowercase or MouseText
    p.sendline("POKE 49167,0")  # Try to enable ALTCHAR
    p.expect_exact("]")
    p.sendline("PRINT \"Hello World\"")
    p.expect_exact("HELLO WORLD")  # Should remain uppercase
    p.expect_exact("]")
    p.sendline("PRINT CHR$(65)")
    p.expect_exact("A")  # Should display normal 'A', not MouseText
    p.expect_exact("]")
    return True

@bobbin('-m twoey')
def test_tty_unenhanced_iie_behavior(p):
    """Test TTY interface with unenhanced Apple IIe"""
    p.expect_exact("]")
    # Unenhanced IIe should support lowercase but not MouseText
    p.sendline("POKE 49167,0")  # Enable ALTCHAR
    p.expect_exact("]")
    p.sendline("PRINT \"Hello World\"")
    p.expect_exact("Hello World")  # Should display mixed case
    p.expect_exact("]")
    # But uppercase letters should not become MouseText
    p.sendline("PRINT \"HELLO\"")
    p.expect_exact("HELLO")  # Should display normal uppercase
    p.expect_exact("]")
    return True

@bobbin('-m enhanced')
def test_tty_special_characters(p):
    """Test special character handling in TTY interface"""
    p.expect_exact("]")
    # Test various symbols and punctuation
    p.sendline("PRINT \"!@#$%^&*()_+\"")
    p.expect_exact("!@#$%^&*()_+")
    p.expect_exact("]")
    # Test with ALTCHAR enabled
    p.sendline("POKE 49167,0")
    p.expect_exact("]")
    p.sendline("PRINT \"@HELLO@\"")  
    p.expect_exact("@@@@@@@@")  # Both @ and letters become MouseText
    p.expect_exact("]")
    return True

@bobbin('-m enhanced')
def test_tty_basic_programming_display(p):
    """Test BASIC programming constructs display correctly in TTY"""
    p.expect_exact("]")
    p.sendline("POKE 49167,0")  # Enable ALTCHAR for mixed case
    p.expect_exact("]")
    # Test BASIC keywords with mixed case input
    p.sendline("10 print \"hello world\"")
    p.expect_exact("10 print \"hello world\"")
    p.expect_exact("]")
    # Test running the program
    p.sendline("run")
    p.expect_exact("hello world")
    p.expect_exact("]")
    # Test LIST command
    p.sendline("list")
    p.expect_exact("10  print \"hello world\"")
    p.expect_exact("]")
    return True