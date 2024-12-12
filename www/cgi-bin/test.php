#!/usr/bin/env php-cgi
<?php
header("Content-Type: text/html");
echo "<html><body>";
echo "<h1>PHP CGI Test</h1>";

// Print environment variables
echo "<h2>Environment Variables:</h2>";
echo "<ul>";
foreach ($_SERVER as $key => $value) {
    echo "<li>$key: $value</li>";
}
echo "</ul>";

// Print POST data
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    echo "<h2>POST Data:</h2>";
    echo "<ul>";
    foreach ($_POST as $key => $value) {
        echo "<li>$key: $value</li>";
    }
    echo "</ul>";
}

echo "</body></html>";
?>
