#!/usr/bin/perl

# Create a C file with the code for quick prime checking by a lookup table.

$table_limit = 65537;	# The largest number to be entered into the table

$verbose = 1;	# whether to print progress bar

$steps_done = 0;	# progress bar

# $chunk_size is the number of bits that we can access from perl at once
$chunk_size = 32;
# we are using a list of long integers for storage
@table = ();
for(0 .. (int($table_limit/$chunk_size/2)))
{
	$table[$_] = 0;
}

# main script

print STDERR "Generating all prime numbers up to $table_limit:\n" if ($verbose);

fill_table();

print_table();

print STDERR "100%\n" if ($verbose);

# Return 1 if the given number is already marked as prime in the table
sub check_prime_from_table
{
	my ($p) = (@_);
	my ($index, $field);
	if ($p > 2 and $p <= $table_limit and $p % 2 == 1)
	{
		$p = int($p/2);
		$index = int ($p / $chunk_size);
		$field = $p % $chunk_size;
		return $table[$index] & (1 << $field);
	}
	else
	{
		return 0;
	}
}

# Mark a given number as prime in the table
sub mark_as_prime
{
	my ($p) = (@_);
	my ($index, $field);
	if ($p > 2 and $p <= $table_limit and $p % 2 == 1)
	{
		if ($verbose and $steps_done*$table_limit < 10*$p)
		{
			printf STDERR "%2d%% ... ", $steps_done*10;
			++$steps_done;
		}
		$p = int($p/2);
		$index = int ($p / $chunk_size);
		$field = $p % $chunk_size;
		$table[$index] |= (1 << $field);
	}
}


sub next_pseudoprime
{
	my ($p) = (@_);
	$p += 2;
	$p += 2 if ($p % 3 == 0);	# obtain next weak pseudoprime
	return $p;
}

sub check_if_prime
{
	my ($p) = (@_);
	my $limit = int(sqrt($p));
	my $divisor = 3;
	return 1 if ($p==2 or $p==3 or $p==5);
	return 0 if ($p <= 1 or ($p%2==0) or ($p%3==0) or ($p%5==0));
	# try all pseudoprime divisors up to sqrt(p)
	while ($divisor <= $limit and $p % $divisor != 0)
	{
		$divisor += 2;
# do not do this, it slows things down a lot
#		$divisor = next_pseudoprime($divisor);
#		while (not check_prime_from_table($divisor))
#		{
#			$divisor = next_pseudoprime($divisor);
#		}
	}
	return ($divisor > $limit) ? 1 : 0;
}

# Go through all numbers to $table_limit and mark them
sub fill_table
{
	my $p = 3;
	while($p <= $table_limit)
	{	# current number $p is prime, we need to find the next one
		mark_as_prime($p);
		last if ($p > $table_limit);
		$p = next_pseudoprime($p);
		while((not check_if_prime($p)) and $p <= $table_limit)
		{
			$p = next_pseudoprime($p);
		}
	}
}

# print table as a C array
sub print_table
{
#	my $index;
	print << "EOF1";
/* This file is generated by "mkfastprimes.pl".
 * It contains a bit-mapped table of prime numbers up to $table_limit
 * and a function to test whether a small number is prime by a table lookup.
 */
const unsigned long primes_table_limit = $table_limit;
const unsigned long primes_table[] = {
EOF1
	for (@table)
	{
		printf "0x%08x,\n", $_;
	}
	print << "EOF2";
};

/* subroutine returns 1 if the number is in the table of prime numbers up to $table_limit */
unsigned primes_table_check(unsigned p)
{
	unsigned index;
	unsigned field;
	if (p==2) return 1;
	if (p<2 || p>primes_table_limit || (p & 1) == 0) return 0;
	p >>= 1;
	index = p >> 5;
	field = p & 31;
	return ((primes_table[index] & (1 << field))==0) ? 0 : 1;
}
/* Testing: run 'mkfastprimes.pl > fastprimes.c' and then compile the following file: */
/*
	#include <stdio.h>
	#include "fastprimes.c"
	int main()
	{
		unsigned i;
		printf("Primes up to %d:\n", primes_table_limit);
		for (i=0; i<=primes_table_limit; i++)
			if (primes_table_check(i)) printf("%d\n", i); 
	}
*/

EOF2
	
}
