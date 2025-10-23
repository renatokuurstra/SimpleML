# SimpleML
a very, very simple ML library for Unreal Engine. I will add as much as I need but that's about it. 

## Neural networks
Using eigen (tensor library headers only in c++) each neural network will allocate all their weights/biases in a single block. This is due to the limited dimensions of these networks, and me wanting to use genetic algoritmh to train the networks.
Currently using a template pattern to specificy how the neuron behave, because later on I want to introduce LSTM and Gru cells. I will also need to think how to introduce recurrency, but for a first pass just feedforward fully connected networks it is.

## Back propagation
Currently not implemented, this is relatively easy to implement. The issue comes in the parametrization and malleability to allow trying different tecniques.

## Genetic algoritmh
As I love these... I will probably go all in:
Multi fitness
Islands
Real time vs Generational
More to come :)