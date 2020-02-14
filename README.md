# hse2prs

**Usage**: `hse2prs [options] file...`

**Options**:
 - `-h`,`--help` Display some basic information about these options
 - `--version` Display compiler version information
 - `-s`,`--stream` Print the results to standard out
 - `-v`,`--verbose` Print all steps taken in simulation
 - `-l`,`--label` Place the id of each node next to it as an external label
 - `-u`,`--unique` Insert state variable transitions to ensure that the resulting set of states have unique predicates
 - `-c`,`--check` Display states with conflicting predicates

Input file syntax is that of graphiz dot, so you can directly render both the input and the result to an image for easy viewing. The input is split into models. Each model contains a set of connected processes, some of which represent a general environment. Processes are specified by subgraphs.

## Basic Graph Attributes

Within each subgraph, a global graph attribute list should be specified. The list of valid 
and/or required attributes are as follows:

```
label
```

If this is set, the value will be added to the front of every variable name within this subgraph `<value>.<name>`.

```
variables
```

This specifies the list of variables contained within this process separated by commas. If this isn't specified, then it will try to derive this list from the reset expression.

```
reset
```

The value of each variable at reset specified as a boolean expression.

```
type {local, remote}
```

If a process does not need to be elaborated, has no internal communication, and only communicates with one other process in the model, then it can be considered remote. Remote processes are able to simulate multiple possible event orderings at the same time saving CPU cycles.

```
elaborate {true, false}
```

If you want to know the possible state encodings for each place, then set this to true.

```
assume
```

May be used multiple times to specify relations that are assumed to be true throughout the entire state space. These relations are forced to be true in the simulation and if an assumption directly conflicts information gained through simulation, there will be an error.

```
assert
```
May be used multiple times to specify relations that should remain true throughout simulation. These relations are not forced, however if there is any possibility that these relations could evaluate to false an error is thrown.

```
parallel_nodes
```

This is not actually necessary for hse2prs since this data is generated in hse2prs and passed down to states2prs. It is only necessary for input to states2prs. It consists of a set of pairs of nodes that are composed in parallel in the format "S0S1 T6S8 ...". This will eventually not be necessary at all and is currently acting as a bandaid to a larger problem.

## Specifying Places

Places must be named S followed by a number. For example, S10, S5, S1, etc. Indices should be non-negative and should start at 0. So the first place in a model should be S0. Indices do not reset for different processes in the same model. So S10 in one process is the same node as S10 in another. Furthermore, nodes should not be shared across processes. There are a couple of attributes that can be applied to places, they are as follows:

```
label
```

The set of state encodings associated with this place specified as a boolean expression.

```
peripheries
```

Places which have more than one periphery are put into the initial marking set as reset nodes.

```
style {filled}
```

Places with a filled style are put into the initial marking set as reset nodes

```
assume
```

May be used multiple times to specify relations that are assumed to be true in only this place. These relations are forced to be true in the simulation and if an assumption directly conflicts information gained through simulation, there will be an error.

```
assert
```

May be used multiple times to specify relations that should remain true in only this place. These relations are not forced, however if there is any possibility that these relations could evaluate to false an error is thrown.

## Specifying Transitions

Transitions must be named T followed by a number. For example, T10, T5, T1, etc. Indices should be non-negative and should start at 0. Again, indices do not reset for different processes in the same model. So T10 in one process is the same node as T10 in another. Furthermore, nodes should not be shared across processes. There are a couple of attributes that can be applied to transitions they are as follows:

```
label
```

REQUIRED!! This specifies the action taken by the transition in a boolean expression. If this expression is surrounded by square brackets "[ expr ]", then it is considered to be a passive transition or "guard".

## Connecting Places and Transitions
Arcs are specified the same way as arcs in dot. There are no special attributes to be considered. The only requirement is that places cannot be connected to other places and transitions cannot be connnected other transitions.

IMPORTANT!!!

There is a conflict case that is currently not detected, and therefore not handled. Right now, conflicts are resolved by breaking an arc and inserting a transition and a place. However if a transition has both multiple inputs and multiple outputs, there may be a conflict that cannot be detected because there is no place to check. Just be wary of this case. It shows up in the pcfb full adder:

```
node<1> en := 1;
*[
    (
        [       S.e & (A.r.t & B.r.f & Ci.r.f | A.r.f & B.r.t & Ci.r.f | A.r.f & B.r.f & Ci.r.t | A.r.t & B.r.t & Ci.r.t) -> S.r.t+
        []      S.e & (A.r.t & B.r.t & Ci.r.f | A.r.t & B.r.f & Ci.r.t | A.r.f & B.r.t & Ci.r.t | A.r.f & B.r.f & Ci.r.f) -> S.r.f+
        ] ||
        [   Co.e & (A.r.t & B.r.t & Ci.r.f | A.r.t & B.r.f & Ci.r.t | A.r.f & B.r.t & Ci.r.t | A.r.t & B.r.t & Ci.r.t) -> Co.r.t+
        []  Co.e & (A.r.t & B.r.f & Ci.r.f | A.r.f & B.r.t & Ci.r.f | A.r.f & B.r.f & Ci.r.t | A.r.f & B.r.f & Ci.r.f) -> Co.r.f+
        ]
    ); {RIGHT HERE} (A.e- || B.e- || Ci.e-); en-;
    (
        ([~S.e]; (S.r.t- || S.r.f-)) ||
        ([~Co.e]; (Co.r.t- || Co.r.f-)) ||
        ([~A.r.t & ~A.r.f & ~B.r.t & ~B.r.f & ~Ci.r.t & ~Ci.r.f]; {AND RIGHT HERE} (A.e+ ||B.e+ ||Ci.e+))
    );
    en+
]
```

This may be fixed by creating a state variable manually

```
node<1> Fe := 1;
node<1> en := 1;
*[
    (
        [       S.e & (A.r.t & B.r.f & Ci.r.f | A.r.f & B.r.t & Ci.r.f | A.r.f & B.r.f & Ci.r.t | A.r.t & B.r.t & Ci.r.t) -> S.r.t+
        []      S.e & (A.r.t & B.r.t & Ci.r.f | A.r.t & B.r.f & Ci.r.t | A.r.f & B.r.t & Ci.r.t | A.r.f & B.r.f & Ci.r.f) -> S.r.f+
        ] ||
        [   Co.e & (A.r.t & B.r.t & Ci.r.f | A.r.t & B.r.f & Ci.r.t | A.r.f & B.r.t & Ci.r.t | A.r.t & B.r.t & Ci.r.t) -> Co.r.t+
        []  Co.e & (A.r.t & B.r.f & Ci.r.f | A.r.f & B.r.t & Ci.r.f | A.r.f & B.r.f & Ci.r.t | A.r.f & B.r.f & Ci.r.f) -> Co.r.f+
        ]
    ); Fe+; (A.e- || B.e- || Ci.e-); en-;
    (
        ([~S.e]; (S.r.t- || S.r.f-)) ||
        ([~Co.e]; (Co.r.t- || Co.r.f-)) ||
        ([~A.r.t & ~A.r.f & ~B.r.t & ~B.r.f & ~Ci.r.t & ~Ci.r.f]; Fe-; (A.e+ ||B.e+ ||Ci.e+))
    );
    en+
]
```


## License

This project is part of the Haystack synthesis engine.

Licensed by Cornell University under GNU GPL v3.

Written by Ned Bingham.
Copyright © 2020 Cornell University.

Haystack is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Haystack is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License may be found in COPYRIGHT.
Otherwise, see <https://www.gnu.org/licenses/>.

