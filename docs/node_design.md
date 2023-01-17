# General Design

The class `Node` is used to represent a node in a node graph, which is defined in `node.h`. The core data of a node is stored in the following member variables of the `Node` class:
```c++
struct Node {
    std::vector<Pin> inputs;  // the input pins of the node
    std::vector<Pin> outputs;  // the output pins of the node
    NodeDataVariant data;  // the data of the node itself
    // ...
};
```
The data of a pin is stored in the member variable `default_value` of the `Pin` class.
```c++
struct Pin {
	PinVariant default_value;  // the data of the pin
    // ...
};
```
Given any node, if `inputs[i]` is unconnected, `inputs[i].default_value` will be viewed as the logical input of the node. If `inputs[i]` is connected with another output pin `outputs[j]`, then `outputs[j].default_value` will be viewed as the logical input of the node. 

# Pin Data Class

In `gui_node_pin_data.h` we define `PinVariant` contains the following types:
```c++
using PinTypeList = TypeList<
	TextureIdData,
	FloatData,
	IntData,
	BoolData,
	Color4Data,  // 4-channel float color
	EnumData,  // enum stored as uint32_t
	ColorRampData,  // 1D color LUT
	FloatTextureIdData,  // FloatData or TextureIdData
	Color4TextureIdData  // Color4Data or TextureIdData
>;
using PinVariant = PinTypeList::cast_to<std::variant>;
```
The subtype of `PinTypeList` is called "pin data class". Every pin data class 
consists of two parts of data: 
- value data: numerical values, which can be uploaded to the uniform buffer on GPU
- auxiliary data: the auxiliary data of the pin.

Value data is necessary. Every pin data class is required to have the following members:
- member type `value_t`: the type of the value data of the pin
- member variable `value_t value`: the value data of the pin

Auxiliary data is optional and unconstrained.


# Node Meta Class

The compile time information about a node named `XXX` is stored in a `NodeXXX` class, which is called "node meta class". All nodes can be divided into three categories: `Image`, `Value` and `Shader`. Which category a node belongs to is determined by the following inheritance relationship:

- `NodeTypeBase`: the base class of all nodes
  - `NodeTypeImageBase`: the base class of all nodes in the `Image` category
    - `NodeBlend`: the class of the `Blend` node
    - `NodeNoise`: the class of the `Noise` node
    - ...
  - `NodeTypeValueBase`: the base class of all nodes in the `Value` category
    - `NodeAdd`: the class of the `Add` node
    - ...
  - `NodeTypeShaderBase`: the base class of all nodes in the `Shader` category
    - `NodePbrShader`: the class of the `PbrShader` node
    - ...
  
All the node meta classes are defined in the `node_XXX.h` file. A node meta class must have the following members:
- member class `UBO`: defines the input pins of the node
- member type `data_type`: defines the type of the node data, which is a subtype of `NodeDataVariant`
- static member function `constexpr char* static name()`: declares the name of the node


