#!/bin/bash
# Test script for crypto commands
# This demonstrates the crypto functionality

echo "=== Testing SecureSysCLI Crypto Commands ==="
echo ""

# Create a test file
echo "Creating test file..."
echo "This is a secret message for testing encryption!" > test_secret.txt
echo "Test file created: test_secret.txt"
echo ""

# Test checksum
echo "=== Test 1: Computing checksum ==="
echo "Command: checksum test_secret.txt"
echo "admin" | ./project << 'EOF'
admin
password
checksum test_secret.txt
exit
EOF

echo ""
echo "=== Test 2: Encrypting file ==="
echo "Command: encrypt test_secret.txt test_secret.enc"
echo "Note: You'll need to enter a password when prompted"
echo ""
echo "To test encryption manually, run:"
echo "  ./launch.sh"
echo "  Then type: encrypt test_secret.txt test_secret.enc"
echo "  Enter password when prompted (input will be hidden)"
echo ""
echo "=== Test 3: Decrypting file ==="
echo "Command: decrypt test_secret.enc test_decrypted.txt"
echo "Note: You'll need to enter the same password"
echo ""
echo "To test decryption manually, run:"
echo "  ./launch.sh"
echo "  Then type: decrypt test_secret.enc test_decrypted.txt"
echo "  Enter the same password when prompted"
echo ""
echo "=== Verification ==="
echo "After decryption, verify with:"
echo "  checksum test_secret.txt"
echo "  checksum test_decrypted.txt"
echo "  (Both should have the same checksum)"

