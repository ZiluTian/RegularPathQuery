# RegularPathQuery

A graph DB for regular path queries. A prototype to demonstrate two different approaches: pg and ospg (output-sensitive).

Prototype behind:

> Mahmoud Abo Khamis, Alexandru-Mihai Hurjui, Ahmet Kara, Dan Olteanu, Dan Suciu, Zilu Tian. *"Output-sensitive evaluation of acyclic conjunctive regular path queries."* ICDT 2026 (to appear).

Instructions for running tests. At the root directory of the project:
```
mkdir build
cd build
cmake ..
make
ctest
```

Suggested readings:

1. Pablo Barceló. "Querying Graph Databases." [Read the paper](https://pbarcelo.ing.uc.cl/pods001i-barcelo.pdf)
2. Mahmoud Abo Khamis, et.al. "Output-Sensitive Evaluation of Regular Path Queries." [Read the paper](https://arxiv.org/pdf/2412.07729)
