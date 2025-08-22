#!/usr/bin/python

import pexpect
import sys
import time

# Test to debug 80-column display corruption issue
def test_80col_display_issue():
    """Debug 80-column display corruption with DaveX"""
    
    # Boot DaveX HDV disk in TTY mode
    print("Booting DaveX HDV in TTY mode...")
    p = pexpect.spawn('./src/bobbin -m enhanced --hdd disk/davexuiowa/uiowa127.hdv', 
                      echo=False, encoding='ascii', timeout=30)
    
    try:
        # Wait for boot and initial screen
        time.sleep(5)
        
        # Try to capture what's on screen
        print("Attempting to capture screen state...")
        
        # Send some commands to see 80-column mode
        p.send('\r')  # Enter to activate
        time.sleep(1)
        
        # Try to get into 80-column mode
        p.send('PR#3\r')  # Enable 80-column card
        time.sleep(1)
        
        p.send('\x04')  # Ctrl-D to catalog
        time.sleep(2)
        
        # Capture output
        p.expect(pexpect.TIMEOUT, timeout=1)
        output = p.before
        print("Captured output:")
        print(repr(output))
        
        # Look for strange characters at line ends
        lines = output.split('\r\n') if output else []
        print(f"\nFound {len(lines)} lines")
        
        for i, line in enumerate(lines[:10]):  # Check first 10 lines
            if len(line) > 40:
                print(f"Line {i} (len={len(line)}): {repr(line)}")
                if len(line) > 78:
                    print(f"  Right edge: {repr(line[78:])}")
        
    except Exception as e:
        print(f"Error: {e}")
        if hasattr(p, 'before') and p.before:
            print(f"Last output: {repr(p.before)}")
    
    finally:
        p.close()

if __name__ == "__main__":
    test_80col_display_issue()