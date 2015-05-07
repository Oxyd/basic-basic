REM Factorisation

print "Enter a natural number > 1"
input n

if n <= 1 then
    print "Number must be greater than 1"
    stop
end if

let divisor = 2
print "Prime divisors:"
do while divisor <= n
    if n mod divisor = 0 then
        print divisor
        let n = n / divisor
    else
        if divisor * divisor > n then
            let divisor = n
        elseif divisor = 2 then
            let divisor = divisor + 1
        else
            let divisor = divisor + 2
        end if
    end if
loop
