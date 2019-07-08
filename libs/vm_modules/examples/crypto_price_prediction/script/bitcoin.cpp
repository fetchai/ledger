function build_graph(graph: Graph)

var conv1D_1_filters = 8;
var conv1D_1_input_channels = 1;
var conv1D_1_kernel_size = 20;
var conv1D_1_stride = 3;

var keep_prob = 0.5f;

var conv1D_2_filters = 1;
var conv1D_2_input_channels = conv1D_1_filters;
var conv1D_2_kernel_size = 16;
var conv1D_2_stride = 4;

graph.AddPlaceholder("Input");
graph.AddConv1D("hidden_conv1D_1", "Input", conv1D_1_filters, conv1D_1_input_channels, conv1D_1_kernel_size, conv1D_1_stride);
graph.AddDropout("dropout_1", "hidden_conv1D_1", keep_prob);
graph.AddFullyConnected("hidden_conv1D_1", "dropout_1", conv1D_2_filters, conv1D_2_input_channels, conv1D_2_kernel_size, conv1D_2_stride);
endfunction
//
//
//function read_weights(graph: Graph)
//// read in weights
//var file_weights0 = System.Argv(0) +  "/output/keras_aluminium_px_last_us/model_weights/hidden_dense_1/hidden_dense_1_12/kernel:0.csv";
//var file_bias0    = System.Argv(0) +  "/output/keras_aluminium_px_last_us/model_weights/hidden_dense_1/hidden_dense_1_12/bias:0.csv";
//var file_weights1 = System.Argv(0) +  "/output/keras_aluminium_px_last_us/model_weights/hidden_dense_2/hidden_dense_2_4/kernel:0.csv";
//var file_bias1    = System.Argv(0) +  "/output/keras_aluminium_px_last_us/model_weights/hidden_dense_2/hidden_dense_2_4/bias:0.csv";
//var file_weights2 = System.Argv(0) +  "/output/keras_aluminium_px_last_us/model_weights/output_dense/output_dense_12/kernel:0.csv";
//var file_bias2    = System.Argv(0) +  "/output/keras_aluminium_px_last_us/model_weights/output_dense/output_dense_12/bias:0.csv";
//
//var weights0 = read_csv(file_weights0, true);
//var bias0    = read_csv(file_bias0, false);
//var weights1 = read_csv(file_weights1, true);
//var bias1    = read_csv(file_bias1, false);
//var weights2 = read_csv(file_weights2, true);
//var bias2    = read_csv(file_bias2, false);
//
//// load weights into graph
//var sd = graph.StateDict();
//sd.SetWeights("hidden_dense_1" + "_FC_Weights", weights0);
//sd.SetWeights("hidden_dense_1" + "_FC_Bias", bias0);
//sd.SetWeights("hidden_dense_2" + "_FC_Weights", weights1);
//sd.SetWeights("hidden_dense_2" + "_FC_Bias", bias1);
//sd.SetWeights("output_dense" + "_FC_Weights", weights2);
//sd.SetWeights("output_dense" + "_FC_Bias", bias2);
//graph.LoadStateDict(sd);
//endfunction

function main()

if (System.Argc() != 3)
  print("Usage: VM SCRIPT_FILE PATH/TO/INPUT.csv /PATH/TO/LABEL.csv");
  return;
endif

var loader = DataLoader();
loader.AddData("tensor", System.Argv(1), System.Argv(2));

var graph = Graph();
build_graph(graph);

var testing = true;

//read_weights(graph);

while(!loader.IsDone())
    var input_data = loader.GetNext();
    graph.SetInput("num_input", input_data.Data());
    var pred = graph.Evaluate("softmax_3");
    print(pred.ToString());
endwhile



endfunction
