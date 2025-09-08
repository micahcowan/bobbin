BEGIN { st = 0 }

# Ignore all "bus write" traces; they'll mess up our line counts.
/^w / { next }

# No processinng until line 4
NR == 4 { st = 1 }

# If we've stored a quad of lines, and it's a disassembly of $03XX,
# print them all and check the next quad
st == 4 && /^03/ {
  for (i=1; i != 4; ++i) {
    print lines[i]
    lines[st]=""
  }
  print $0
  st = 1
  elips = 0
  next
}

# If we've stored a quad of lines, but we're not in $03XX, elide them.
st == 4 {
  if (!elips) {
    print "[...]"
    elips = 1
  }
  st = 1
  next
}

# Store a line (1-3)
st { lines[st] = $0; ++st; next }
