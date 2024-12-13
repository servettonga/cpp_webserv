#!/bin/bash

# Create main test directories
mkdir -p test_server/put_test
mkdir -p YoupiBanane/nop YoupiBanane/Yeah

# Create test files with content
cat > YoupiBanane/youpi.bad_extension << EOF
This is youpi.bad_extension
EOF

cat > YoupiBanane/youpi.bla << EOF
This is youpi.bla
EOF

cat > YoupiBanane/nop/youpi.bad_extension << EOF
This is nop/youpi.bad_extension
EOF

cat > YoupiBanane/nop/other.pouic << EOF
This is nop/other.pouic
EOF

cat > YoupiBanane/Yeah/not_happy.bad_extension << EOF
This is Yeah/not_happy.bad_extension
EOF

# Set permissions
chmod 755 test_server test_server/put_test YoupiBanane YoupiBanane/nop YoupiBanane/Yeah
chmod 644 YoupiBanane/youpi.bad_extension YoupiBanane/youpi.bla \
    YoupiBanane/nop/youpi.bad_extension YoupiBanane/nop/other.pouic \
    YoupiBanane/Yeah/not_happy.bad_extension

# Copy CGI tester if it exists
if [ -f "ubuntu_cgi_tester" ]; then
    cp ubuntu_cgi_tester test_server/
    chmod +x test_server/ubuntu_cgi_tester
fi