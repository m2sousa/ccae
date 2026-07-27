#include <stdbit.h>

#include <ccae/types.h>
#include <ccae/utils.h>

u64 gcd(u64 a, u64 b) {
	if (a == 0)
		return b;

	if (b == 0)
		return a;

	// how many factor of 2 are shared between `a` and `b`.
	unsigned shift = stdc_trailing_zeros(a | b);

	// from there, a is odd.
	a >>= stdc_trailing_zeros(a);

	while (b != 0) {
		// case odd/even, dividing the even one by 2 does not change the gcd.
		b >>= stdc_trailing_zeros(b);

		// for simplicity: gcd(a,b) = gcd(b,a).
		if (a > b) {
			u64 tmp = a;
			a = b;
			b = tmp;
		}

		// gcd(a,b) = gcd(|a-b|, min(a,b)) => gcd(a,b) = gcd(a, b-a).
		b -= a;
	}

	return a << shift;
}
