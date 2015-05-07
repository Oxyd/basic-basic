REM Factorial computation in structured Basic

print "Enter a natural number"
input n

if n > 0 then
    let f = 1
    for i = 1 to n
        let f = f * i
    next i

    print n, "! = ", f
else
    print n, " is not a natural number"
end if
