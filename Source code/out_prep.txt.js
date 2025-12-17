// Generated JavaScript code from custom language
// Date: Dec 17 2025 04:31:07

// Helper functions for built-in operations
function __timeFled(startStr, endStr) {
    function parseDate(str) {
        if (typeof str !== 'string' || str.length !== 8) return new Date();
        const day = parseInt(str.substring(0, 2), 10);
        const month = parseInt(str.substring(2, 4), 10) - 1;
        const year = parseInt(str.substring(4, 8), 10);
        return new Date(year, month, day);
    }
    const start = parseDate(startStr);
    const end = parseDate(endStr);
    return Math.floor((end.getTime() - start.getTime()) / 1000); // Возвращаем секунды
}

function __unite(count, ...args) {
    let result = '';
    const limit = Math.min(count, args.length);
    for (let i = 0; i < limit; i++) {
        if (args[i] !== undefined && args[i] !== null) {
            result += String(args[i]);
        }
    }
    return result;
}

function __sum4(a, b, c, d) {
    return Number(a) + Number(b) + Number(c) + Number(d);
}

function SumarizE(a, b, c, d) {
    let /* INT */ e = (((a + b) + c) + d);
    return e;
}

function sayHi() {
    console.log("Hi");
    console.log("Gleb");
}

function SayHello() {
    let /* STRING */ hello;
    hello = (("Hello " + "Gleb") + " !");
    console.log(hello);
}

function IsGreater(a, b) {
    return (a > b);
}


// Main execution block
(function() {
    let /* STRING */ message;
    message = "Here everything begins!";
    console.log(message);
    let /* unsigned int */ x;
    let /* unsigned int */ y;
    x = 5;
    y = 10;
    let /* BOOL */ z;
    z = IsGreater(x, y);
    console.log(z);
    let /* STRING */ str1;
    str1 = String(z);
    console.log(str1);
    let /* INT */ i;
    i = 0;
    do {
        message = (("This is " + String(i)) + "th iteration");
        console.log(message);
        i = (i + 1);
    } while ((i < 5));
    let /* TIME_T */ curr;
    curr = Math.floor(Date.now() / 1000);
    console.log(curr);
    let /* TIME_T */ years;
    years = ((curr / 31557600) + 1970);
    console.log("сейчас ");
    console.log(years);
    console.log(" год");
    message = String(__timeFled("16112006", "17122025"));
    let /* INT */ a;
    a = Math.pow((((2 + 3) * 4) - 12), 2);
    message = String(a);
    console.log(message);
    let /* INT */ hex1;
    let /* INT */ hex2;
    hex1 = parseInt('f', 16);
    hex2 = parseInt('a', 16);
    message = String((hex1 + hex2));
    console.log(message);
    a = SumarizE(19, hex2, 3, parseInt('f', 16));
    message = String(a);
    console.log(message);
    let /* INT */ oct1;
    let /* INT */ oct2;
    oct1 = 04;
    oct2 = 07;
    a = (oct1 + oct2);
    message = String(a);
    console.log(message);
    let /* SYMB */ first;
    let /* SYMB */ second;
    let /* SYMB */ third;
    first = "n";
    second = "g";
    third = "s";
    message = __unite(3, first, second, third);
    console.log(message);
    SayHello();
})();
