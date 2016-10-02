<?php
$firmwares = array(
    "pomodoro" => "v0.0.6",
    "xmaslights" => "v0.0.1"
);
$spiffs = array(
    "pomodoro" => "v0.0.3",
    "xmaslights" => "v0.0.0"
);

function check_header($name, $value = false) {
    logMessage("CHECKING: " . $name);
    if (! isset($_SERVER[$name])) {
        return false;
    }
    if ($value && $_SERVER[$name] != $value) {
        return false;
    }
    logMessage("=OK\n");
    return true;
}

function sendFile($path) {
    header($_SERVER["SERVER_PROTOCOL"] . ' 200 OK', true, 200);
    header('Content-Type: application/octet-stream', true);
    header('Content-Disposition: attachment; filename=' . basename($path));
    header('Content-Length: ' . filesize($path), true);
    header('x-MD5: ' . md5_file($path), true);
    readfile($path);
}

function escape_single_quote($string) {
    return str_replace("'", "\\'", $string);
}

function logAccess() {
    logMessage(print_r($_SERVER, true));
    date_default_timezone_set('Europe/Stockholm');
    $row = "'" . date('Y-m-d H:i:s') . "'";
    foreach (getallheaders() as $name => $value) {
        $row .= ",'" . escape_single_quote($name) . "=" . escape_single_quote($value) . "'";
    }
    $row .= "\n";
    logMessage($row, "../../../ota_log/access.csv");
}

function logMessage($message, $file = "../../../ota_log/log.txt") {
    file_put_contents($file, $message, FILE_APPEND | LOCK_EX);
}

// Execution starts here
logAccess();

if (! check_header('HTTP_USER_AGENT', 'ESP8266-http-Update') || ! check_header('HTTP_X_ESP8266_STA_MAC') || ! check_header('HTTP_X_ESP8266_AP_MAC') || ! check_header('HTTP_X_ESP8266_FREE_SPACE') || ! check_header('HTTP_X_ESP8266_CHIP_SIZE') || ! check_header('HTTP_X_ESP8266_SDK_VERSION')) {
    header($_SERVER["SERVER_PROTOCOL"] . ' 403 Forbidden', true, 403);
    echo "only for ESP8266 updater! (header)\n";
    exit();
}

$product = isset($_GET['product']) ? $_GET['product'] : null;

logMessage("Product: " . $product . "\n");

switch ($_SERVER['HTTP_X_ESP8266_MODE']) {
    case "sketch":
        logMessage("Sketch update\n");
        $db = $firmwares;
        $localBinary = "./bin/" . $product . ".bin";
        break;
    case "spiffs":
        logMessage("SPIFFS update\n");
        $db = $spiffs;
        $localBinary = "./bin/" . $product . ".spiffs.bin";
        break;
    default:
        logMessage("Unknown mode\n");
        header($_SERVER["SERVER_PROTOCOL"] . ' 404 Mode not available', true, 404);
        exit();
}

if (! isset($db[$product])) {
    logMessage("Product is missing!" . $product . "\n");
    header($_SERVER["SERVER_PROTOCOL"] . ' 404 Product not found', true, 404);
    exit();
}


if ($db[$_GET['product']] == $_SERVER['HTTP_X_ESP8266_VERSION']) {
    logMessage("No update required\n");
    header($_SERVER["SERVER_PROTOCOL"] . ' 304 Not Modified', true, 304);
    exit();
}

logMessage("Updating using $localBinary \n");
sendFile($localBinary);
logMessage("Update from $localBinary complete\n");
