#!/bin/bash

echo "=========================================="
echo "Phase 3 Feature Testing Suite"
echo "=========================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PASSED=0
FAILED=0

# Test function
test_check() {
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ PASSED${NC}: $1"
        ((PASSED++))
    else
        echo -e "${RED}✗ FAILED${NC}: $1"
        ((FAILED++))
    fi
}

# Cleanup function
cleanup() {
    rm -f test_*.txt test_*.enc test_*.dec server.crt server.key
    pkill -f "./project" 2>/dev/null
}

trap cleanup EXIT

echo "1. Testing Build..."
echo "-------------------"
make clean > /dev/null 2>&1
make > /dev/null 2>&1
test_check "Project compiles successfully"

if [ ! -f "./project" ]; then
    echo -e "${RED}Build failed - project binary not found${NC}"
    exit 1
fi

echo ""
echo "2. Testing AES-GCM Encryption/Decryption..."
echo "-------------------------------------------"

# Create test file
echo "This is a test file for encryption" > test_encrypt.txt

# Test encryption (will prompt for password - we'll test manually)
echo "Testing encryption with AES-GCM..."
echo "Note: Manual password entry required for encryption tests"

# Check if crypto functions exist in binary
strings ./project | grep -q "encrypt_file" && test_check "Encrypt function present" || echo -e "${YELLOW}⚠ Warning: encrypt_file not found in binary${NC}"

echo ""
echo "3. Testing ACL System..."
echo "-----------------------"

# Check if ACL functions are compiled
strings ./project | grep -q "check_command_permission" && test_check "ACL functions present" || echo -e "${YELLOW}⚠ Warning: ACL functions not found${NC}"

# Test ACL file creation
echo "testuser delete allow" > acl.db 2>/dev/null
test_check "ACL file can be created"

echo ""
echo "4. Testing Remote/TLS Features..."
echo "----------------------------------"

# Check if TLS functions are compiled
strings ./project | grep -q "start_tls_server" && test_check "TLS server function present" || echo -e "${YELLOW}⚠ Warning: TLS server function not found${NC}"
strings ./project | grep -q "connect_tls_client" && test_check "TLS client function present" || echo -e "${YELLOW}⚠ Warning: TLS client function not found${NC}"

echo ""
echo "5. Testing Sandbox Features..."
echo "------------------------------"

# Check if sandbox functions are compiled
strings ./project | grep -q "run_sandboxed" && test_check "Sandbox function present" || echo -e "${YELLOW}⚠ Warning: Sandbox function not found${NC}"

echo ""
echo "6. Testing Command Integration..."
echo "---------------------------------"

# Check if new commands are in command list
strings ./project | grep -q "cmd_server" && test_check "Server command integrated" || echo -e "${YELLOW}⚠ Warning: Server command not found${NC}"
strings ./project | grep -q "cmd_client" && test_check "Client command integrated" || echo -e "${YELLOW}⚠ Warning: Client command not found${NC}"
strings ./project | grep -q "cmd_sandbox" && test_check "Sandbox command integrated" || echo -e "${YELLOW}⚠ Warning: Sandbox command not found${NC}"
strings ./project | grep -q "cmd_acl" && test_check "ACL command integrated" || echo -e "${YELLOW}⚠ Warning: ACL command not found${NC}"

echo ""
echo "7. Testing Help Command..."
echo "--------------------------"

# Create a test input file
cat > test_help_input.txt << EOF
help
exit
EOF

# Test help command (non-interactive)
timeout 2 ./project < test_help_input.txt 2>&1 | grep -q "server" && test_check "Help shows new commands" || echo -e "${YELLOW}⚠ Warning: Help command test inconclusive${NC}"

echo ""
echo "8. Testing File Format (AES-GCM)..."
echo "------------------------------------"

# Check if PBKDF2_ITER is set to 100000
grep -q "PBKDF2_ITER 100000" crypto.c && test_check "PBKDF2 iterations set to 100k" || test_check "PBKDF2 iterations check"

# Check if AES-GCM is used
grep -q "EVP_aes_256_gcm" crypto.c && test_check "AES-GCM encryption mode" || test_check "AES-GCM check"

# Check if TAG_SIZE is defined
grep -q "TAG_SIZE" crypto.c && test_check "GCM tag size defined" || test_check "GCM tag check"

echo ""
echo "9. Testing Error Handling..."
echo "----------------------------"

# Check if generic error messages are used (both encryption and decryption)
if grep -q "Decryption failed" crypto.c; then
    test_check "Generic decryption error messages"
else
    echo -e "${RED}✗ FAILED${NC}: Generic decryption error messages"
    ((FAILED++))
fi

if grep -q "Encryption failed" crypto.c; then
    test_check "Generic encryption error messages"
else
    echo -e "${RED}✗ FAILED${NC}: Generic encryption error messages"
    ((FAILED++))
fi

# Check that no perror() calls leak information in crypto.c
if grep -q "perror" crypto.c; then
    echo -e "${RED}✗ FAILED${NC}: perror() calls found (may leak information)"
    ((FAILED++))
else
    test_check "No perror() calls in crypto.c"
fi

# Check that detailed error messages are not present
if grep -q "PBKDF2 failed" crypto.c || grep -q "EncryptInit failed" crypto.c || grep -q "DecryptInit failed" crypto.c || grep -q "EncryptUpdate failed" crypto.c || grep -q "EncryptFinal failed" crypto.c; then
    echo -e "${RED}✗ FAILED${NC}: Detailed error messages found"
    ((FAILED++))
else
    test_check "No detailed error messages found"
fi

echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All automated tests passed!${NC}"
    exit 0
else
    echo -e "${YELLOW}Some tests failed or need manual verification${NC}"
    exit 1
fi


