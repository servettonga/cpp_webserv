#!/bin/bash
# setup.sh

mkdir -p www/{static,cgi-bin,errors,portfolio}
mkdir -p www/static/uploads

# Create error pages
#echo "<h1>404 - Not Found</h1>" > www/errors/404.html
#echo "<h1>403 - Forbidden</h1>" > www/errors/403.html
#echo "<h1>500 - Server Error</h1>" > www/errors/50x.html

# Create index files
#echo "<h1>Welcome to Main Server</h1>" > www/index.html
#echo "<h1>Welcome to Static Files</h1>" > www/static/index.html
#echo "<h1>Welcome to Portfolio</h1>" > www/portfolio/index.html

# Set permissions
chmod -R 755 www
chmod 777 www/static/uploads

# Ensure CGI handlers exist or create symlinks
if [ ! -f /usr/bin/python3 ]; then
    sudo ln -s "$(which python3)" /usr/bin/python3
fi

if [ ! -f /usr/bin/php-cgi ]; then
    sudo ln -s "$(which php-cgi)" /usr/bin/php-cgi
fi

if [ ! -f /usr/bin/perl ]; then
    sudo ln -s "$(which perl)" /usr/bin/perl
fi
