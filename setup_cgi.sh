#!/bin/bash
# setup_cgi.sh

# Function to check and install packages
check_install() {
    local package=$1
    local binary=$2

    if ! command -v $binary &> /dev/null; then
        echo "Installing $package..."
        if [ -f /etc/debian_version ]; then
            sudo apt-get install -y $package
        elif [ -f /etc/redhat-release ]; then
            sudo yum install -y $package
        elif [ -f /etc/arch-release ]; then
            sudo pacman -S --noconfirm $package
        else
            echo "Unsupported distribution"
            exit 1
        fi
    fi
}

# Install required CGI handlers
check_install python3 python3
check_install php-cgi php-cgi
check_install perl perl

# Create symlinks if needed
[ ! -f /usr/bin/python3 ] && sudo ln -s "$(which python3)" /usr/bin/python3
[ ! -f /usr/bin/php-cgi ] && sudo ln -s "$(which php-cgi)" /usr/bin/php-cgi
[ ! -f /usr/bin/perl ] && sudo ln -s "$(which perl)" /usr/bin/perl

# Create CGI directory and test scripts
mkdir -p www/cgi-bin

# Python test script
cat > www/cgi-bin/test.py << 'EOF'
#!/usr/bin/env python3
print("Content-Type: text/html\n")
print("<html><body>")
print("<h1>Python CGI Test</h1>")
print("<p>This is a test of Python CGI!</p>")
print("</body></html>")
EOF

# PHP test script
cat > www/cgi-bin/test.php << 'EOF'
#!/usr/bin/env php-cgi
<?php
header("Content-Type: text/html");
echo "<html><body>";
echo "<h1>PHP CGI Test</h1>";
echo "<p>This is a test of PHP CGI!</p>";
echo "</body></html>";
?>
EOF

# Set proper permissions
chmod 755 www/cgi-bin/*.{py,php}

echo "CGI handlers setup complete"