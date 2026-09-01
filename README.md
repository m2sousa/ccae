# ccae — Canonical Constraint Algebra Engine

A dependency-free computer algebra engine written in C23 for computing Poisson brackets of smeared constraint functionals in canonical gravity and perturbative cosmology.
The engine is designed to be highly generic, which may make it adaptable to other symbolic algebra applications.

The unit tests in `tests/` provide examples of how to use the library. 

For a pedagogical introduction to the motivations and related mathematical objects, see DBM24: [arXiv:2409.06441](https://arxiv.org/pdf/2409.06441).

## Build

Requirements: `make` and a C23-compatible compiler.

```sh
make all      # Build build/libccae.so and build/ccae-tests.
make tests    # Build and run the tests.
```

## Planned improvements

Potential improvements to the library include, off the top on my mind:

- [ ] Add a LaTeX front-end for inputs and presenting results.
- [ ] Expand test coverage with batch tests based on the literature.
- [ ] Support symbols for undetermined functions of tensorial arguments.
- [ ] Implement the Butler–Portugal algorithm for faster tensor canonicalization ([arXiv:1702.08114v3](https://arxiv.org/abs/1702.08114)).
- [ ] Add a Schwartz–Zippel-based oracle ([DOI: 10.1145/322217.322225](https://doi.org/10.1145/322217.322225)).
