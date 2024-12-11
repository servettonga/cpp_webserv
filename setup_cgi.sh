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

echo "CGI handlers setup complete"