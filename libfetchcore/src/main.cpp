#include"fetch_pybind.hpp"

#include "py_logger.hpp"
#include "py_unittest.hpp"
#include "py_assert.hpp"
#include "py_protocols.hpp"
#include "py_abstract_mutex.hpp"
#include "py_mutex.hpp"
#include "crypto/py_hash.hpp"
//#include "crypto/py_prover.hpp"
#include "crypto/py_fnv.hpp"
//#include "crypto/py_merkle_set.hpp"
#include "crypto/py_ecdsa.hpp"
#include "crypto/py_verifier.hpp"
#include "crypto/py_sha256.hpp"
//#include "crypto/py_stream_hasher.hpp"
#include "serializers/py_referenced_byte_array.hpp"
#include "serializers/py_serializable_exception.hpp"
#include "serializers/py_counter.hpp"
#include "serializers/py_typed_byte_array_buffer.hpp"
#include "serializers/py_byte_array_buffer.hpp"
#include "serializers/py_type_register.hpp"
#include "serializers/py_stl_types.hpp"
#include "serializers/py_exception.hpp"
#include "memory/py_array.hpp"
#include "memory/py_shared_array.hpp"

#include "memory/py_rectangular_array.hpp"
#include "network/py_tcp_server.hpp"
#include "network/py_thread_manager.hpp"
//#include "network/py_tcp_client.hpp"
#include "network/py_message.hpp"
#include "network/tcp/py_client_manager.hpp"
//#include "network/tcp/py_abstract_server.hpp"
//#include "network/tcp/py_client_connection.hpp"
//#include "network/tcp/py_abstract_connection.hpp"
//#include "meta/py_log2.hpp"
#include "chain/py_transaction.hpp"
#include "chain/py_block_generator.hpp"
#include "chain/py_block.hpp"
#include "chain/consensus/py_proof_of_work.hpp"
#include "commandline/py_vt100.hpp"
//#include "commandline/py_parameter_parser.hpp"
#include "math/py_exp.hpp"
#include "math/py_bignumber.hpp"
#include "math/py_log.hpp"
#include "math/linalg/py_matrix.hpp"
#include "math/spline/py_linear.hpp"
#include "script/py_variant.hpp"
#include "image/py_load_png.hpp"
#include "image/py_image.hpp"
#include "storage/py_versioned_random_access_stack.hpp"
#include "storage/py_file_object.hpp"
#include "storage/py_variant_stack.hpp"
#include "storage/py_random_access_stack.hpp"
#include "storage/py_indexed_document_store.hpp"
#include "byte_array/py_referenced_byte_array.hpp"
#include "byte_array/py_encoders.hpp"
#include "byte_array/py_const_byte_array.hpp"
#include "byte_array/py_basic_byte_array.hpp"
#include "byte_array/py_consumers.hpp"
#include "byte_array/py_decoders.hpp"
#include "byte_array/tokenizer/py_token.hpp"
#include "byte_array/tokenizer/py_tokenizer.hpp"
#include "byte_array/details/py_encode_decode.hpp"
#include "json/py_document.hpp"
#include "json/py_exceptions.hpp"
/*
#include "http/py_key_value_set.hpp"
#include "http/py_connection.hpp"
#include "http/py_server.hpp"
#include "http/py_session.hpp"
#include "http/py_abstract_server.hpp"
#include "http/py_response.hpp"
#include "http/py_http_connection_manager.hpp"
#include "http/py_method.hpp"
#include "http/py_module.hpp"
#include "http/py_status.hpp"
#include "http/py_header.hpp"
#include "http/py_request.hpp"
#include "http/py_abstract_connection.hpp"
#include "http/py_route.hpp"
#include "http/py_middleware.hpp"
#include "http/py_view_parameters.hpp"
#include "http/py_query.hpp"
#include "http/py_mime_types.hpp"
#include "http/middleware/py_allow_origin.hpp"
#include "http/middleware/py_color_log.hpp"
*/
#include "string/py_trim.hpp"
#include "containers/py_vector.hpp"
/*
#include "service/py_abstract_callable.hpp"
#include "service/py_protocol.hpp"
#include "service/py_promise.hpp"
#include "service/py_error_codes.hpp"
#include "service/py_server.hpp"
#include "service/py_function.hpp"
#include "service/py_client_interface.hpp"
#include "service/py_feed_subscription_manager.hpp"
#include "service/py_abstract_publication_feed.hpp"
#include "service/py_publication_feed.hpp"
#include "service/py_callable_class_member.hpp"
#include "service/py_server_interface.hpp"
#include "service/py_message_types.hpp"
#include "service/py_types.hpp"
#include "service/py_client.hpp"
*/
#include "random/py_bitmask.hpp"
#include "random/py_lfg.hpp"
#include "random/py_bitgenerator.hpp"
#include "random/py_lcg.hpp"
/*
#include "protocols/py_swarm.hpp"
#include "protocols/py_fetch_protocols.hpp"
#include "protocols/py_chain_keeper.hpp"
#include "protocols/swarm/py_controller.hpp"
#include "protocols/swarm/py_protocol.hpp"
#include "protocols/swarm/py_serializers.hpp"
#include "protocols/swarm/py_entry_point.hpp"
#include "protocols/swarm/py_commands.hpp"
#include "protocols/swarm/py_node_details.hpp"
#include "protocols/chain_keeper/py_chain_manager.hpp"
#include "protocols/chain_keeper/py_controller.hpp"
#include "protocols/chain_keeper/py_protocol.hpp"
#include "protocols/chain_keeper/py_commands.hpp"
#include "protocols/chain_keeper/py_transaction_manager.hpp"
*/

// #include "optimisation/py_abstract_spinglass_solver.hpp"
#include "optimisation/instance/py_load_txt.hpp"
#include "optimisation/instance/py_binary_problem.hpp"
#include "optimisation/simulated_annealing/py_reference_annealer.hpp"
#include "optimisation/simulated_annealing/py_sparse_annealer.hpp"


PYBIND11_MODULE(libfetchcore, module) {
  namespace py = pybind11;

// Namespaces
  py::module ns_fetch = module.def_submodule("fetch");
  py::module ns_fetch_serializers = ns_fetch.def_submodule("serializers");
  py::module ns_fetch_chain = ns_fetch.def_submodule("chain");
  py::module ns_fetch_chain_consensus = ns_fetch_chain.def_submodule("consensus");
  py::module ns_fetch_unittest = ns_fetch.def_submodule("unittest");
  py::module ns_fetch_protocols = ns_fetch.def_submodule("protocols");
  py::module ns_fetch_random = ns_fetch.def_submodule("random");
  py::module ns_fetch_log = ns_fetch.def_submodule("log");
  py::module ns_fetch_log_details = ns_fetch_log.def_submodule("details");
  py::module ns_fetch_service = ns_fetch.def_submodule("service");
  py::module ns_fetch_service_details = ns_fetch_service.def_submodule("details");
  py::module ns_fetch_vectorize = ns_fetch.def_submodule("vectorize");
  py::module ns_fetch_json = ns_fetch.def_submodule("json");
  py::module ns_fetch_crypto = ns_fetch.def_submodule("crypto");
  py::module ns_fetch_image = ns_fetch.def_submodule("image");
  py::module ns_fetch_image_colors = ns_fetch_image.def_submodule("colors");
  py::module ns_fetch_math = ns_fetch.def_submodule("math");
  py::module ns_fetch_math_spline = ns_fetch_math.def_submodule("spline");
  py::module ns_fetch_network = ns_fetch.def_submodule("network");
  py::module ns_fetch_memory = ns_fetch.def_submodule("memory");
  py::module ns_fetch_memory_details = ns_fetch_memory.def_submodule("details");
  py::module ns_fetch_storage = ns_fetch.def_submodule("storage");
  py::module ns_fetch_optimisers = ns_fetch.def_submodule("optimisers");
  py::module ns_fetch_commandline = ns_fetch.def_submodule("commandline");
  py::module ns_fetch_script = ns_fetch.def_submodule("script");
  py::module ns_fetch_byte_array = ns_fetch.def_submodule("byte_array");
  py::module ns_details = module.def_submodule("details");
  py::module ns_details_meta = ns_details.def_submodule("meta");
  py::module ns_fetch_mutex = ns_fetch.def_submodule("mutex");
  py::module ns_fetch_containers = ns_fetch.def_submodule("containers");
  py::module ns_fetch_math_linalg = ns_fetch_math.def_submodule("linalg");
  py::module ns_fetch_http = ns_fetch.def_submodule("http");
  py::module ns_fetch_optimisation = ns_fetch.def_submodule("optimisation");

  // TODO: delete    BuildMerkleSet(ns_fetch_crypto)
  // TODO: delete    BuildSharedHashTable(ns_fetch_memory)
  // TODO: delete    BuildRecord(ns_fetch_memory_details)
// Objects
/*     
   // Crypto stuff
   BuildProver(ns_fetch_crypto)
   BuildFNV(ns_fetch_crypto)   
   BuildECDSASigner(ns_fetch_crypto)
   BuildSHA256(ns_fetch_crypto)
   BuildStreamHasher(ns_fetch_crypto)
*/

  fetch::memory::BuildArray<int8_t>("ArrayInt8", ns_fetch_memory);      
  fetch::memory::BuildArray<int16_t>("ArrayInt16", ns_fetch_memory);    
  fetch::memory::BuildArray<int32_t>("ArrayInt32", ns_fetch_memory);
  fetch::memory::BuildArray<int64_t>("ArrayInt64", ns_fetch_memory);

  fetch::memory::BuildArray<uint8_t>("ArrayUInt8", ns_fetch_memory);      
  fetch::memory::BuildArray<uint16_t>("ArrayUInt16", ns_fetch_memory);    
  fetch::memory::BuildArray<uint32_t>("ArrayUInt32", ns_fetch_memory);
  fetch::memory::BuildArray<uint64_t>("ArrayUInt64", ns_fetch_memory);
  
  fetch::memory::BuildArray<float>("ArrayFloat", ns_fetch_memory);
  fetch::memory::BuildArray<double>("ArrayDouble", ns_fetch_memory);  


  fetch::memory::BuildSharedArray<int8_t>("SharedArrayInt8", ns_fetch_memory);      
  fetch::memory::BuildSharedArray<int16_t>("SharedArrayInt16", ns_fetch_memory);    
  fetch::memory::BuildSharedArray<int32_t>("SharedArrayInt32", ns_fetch_memory);
  fetch::memory::BuildSharedArray<int64_t>("SharedArrayInt64", ns_fetch_memory);

  fetch::memory::BuildSharedArray<uint8_t>("SharedArrayUInt8", ns_fetch_memory);      
  fetch::memory::BuildSharedArray<uint16_t>("SharedArrayUInt16", ns_fetch_memory);    
  fetch::memory::BuildSharedArray<uint32_t>("SharedArrayUInt32", ns_fetch_memory);
  fetch::memory::BuildSharedArray<uint64_t>("SharedArrayUInt64", ns_fetch_memory);
  
  fetch::memory::BuildSharedArray<float>("SharedArrayFloat", ns_fetch_memory);
  fetch::memory::BuildSharedArray<double>("SharedArrayDouble", ns_fetch_memory);  
  
  fetch::memory::BuildRectangularArray<int8_t>("RectangularArrayInt8", ns_fetch_memory);      
  fetch::memory::BuildRectangularArray<int16_t>("RectangularArrayInt16", ns_fetch_memory);    
  fetch::memory::BuildRectangularArray<int32_t>("RectangularArrayInt32", ns_fetch_memory);
  fetch::memory::BuildRectangularArray<int64_t>("RectangularArrayInt64", ns_fetch_memory);

  fetch::memory::BuildRectangularArray<uint8_t>("RectangularArrayUInt8", ns_fetch_memory);      
  fetch::memory::BuildRectangularArray<uint16_t>("RectangularArrayUInt16", ns_fetch_memory);    
  fetch::memory::BuildRectangularArray<uint32_t>("RectangularArrayUInt32", ns_fetch_memory);
  fetch::memory::BuildRectangularArray<uint64_t>("RectangularArrayUInt64", ns_fetch_memory);
  
  fetch::memory::BuildRectangularArray<float>("RectangularArrayFloat", ns_fetch_memory);
  fetch::memory::BuildRectangularArray<double>("RectangularArrayDouble", ns_fetch_memory);  

  
/*
// Network stuff
   BuildTCPServer(ns_fetch_network)
   BuildThreadManager(ns_fetch_network)
   BuildTCPClient(ns_fetch_network)
   BuildClientManager(ns_fetch_network)
   BuildAbstractNetworkServer(ns_fetch_network)
   BuildClientConnection(ns_fetch_network)
   BuildAbstractClientConnection(ns_fetch_network)
   BuildLog2(ns_details_meta)
   BuildTransaction(ns_fetch_chain)
   BuildBlockGenerator(ns_fetch_chain)
   BuildBasicBlock(ns_fetch_chain)
   BuildProofOfWork(ns_fetch_chain_consensus)
   BuildParamsParser(ns_fetch_commandline)
*/
//  fetch::math::BuildExp< 0, 60801, false>("Exp0", ns_fetch_math);  
//  fetch::math::BuildBigUnsigned(ns_fetch_math);
//  fetch::math::BuildLog(ns_fetch_math);

//  fetch::math::linalg::BuildMatrix<int8_t>("MatrixInt8", ns_fetch_math_linalg);      
//  fetch::math::linalg::BuildMatrix<int16_t>("MatrixInt16", ns_fetch_math_linalg);    
//  fetch::math::linalg::BuildMatrix<int32_t>("MatrixInt32", ns_fetch_math_linalg);
  //  fetch::math::linalg::BuildMatrix<int64_t>("MatrixInt64", ns_fetch_math_linalg);

  //  fetch::math::linalg::BuildMatrix<uint8_t>("MatrixUInt8", ns_fetch_math_linalg);      
  //  fetch::math::linalg::BuildMatrix<uint16_t>("MatrixUInt16", ns_fetch_math_linalg);    
  //  fetch::math::linalg::BuildMatrix<uint32_t>("MatrixUInt32", ns_fetch_math_linalg);
  //  fetch::math::linalg::BuildMatrix<uint64_t>("MatrixUInt64", ns_fetch_math_linalg);
  
  fetch::math::linalg::BuildMatrix<float>("MatrixFloat", ns_fetch_math_linalg);
  fetch::math::linalg::BuildMatrix<double>("MatrixDouble", ns_fetch_math_linalg);  

//  fetch::math::BuildSpline(ns_fetch_math_spline);
  

/*
   BuildDictionary(ns_fetch_script)
   BuildFunction(ns_fetch_script)
   BuildVariant(ns_fetch_script)
   BuildVariant(ns_fetch_script)
   BuildAbstractSyntaxTree(ns_fetch_script)
*/
  
//  fetch::image::BuildFileReadErrorException(ns_fetch_image);

  fetch::image::colors::BuildAbstractColor<uint32_t, 8, 3>("ColorRGB8",ns_fetch_image_colors);
  fetch::image::colors::BuildAbstractColor<uint32_t, 8, 4>("ColorRGBA8",ns_fetch_image_colors);  

//  fetch::image::BuildImageType< fetch::image::colors::RGB8 >("ImageRGB8", ns_fetch_image);
//  fetch::image::BuildImageType< fetch::image::colors::RGBA8 >("ImageRGBA8", ns_fetch_image);  
  

/*
   BuildVersionedRandomAccessStack(ns_fetch_storage)
   BuildFileObjectImplementation(ns_fetch_storage)
   BuildVariantStack(ns_fetch_storage)
   BuildRandomAccessStack(ns_fetch_storage)
   BuildIndexedDocumentStore(ns_fetch_storage)
*/
  
  fetch::byte_array::BuildBasicByteArray(ns_fetch_byte_array);
  fetch::byte_array::BuildByteArray(ns_fetch_byte_array);
  fetch::byte_array::BuildConstByteArray(ns_fetch_byte_array);

  /*
// Serializers
   BuildTypedByte_ArrayBuffer(ns_fetch_serializers)
   BuildSizeCounter(ns_fetch_serializers)
   BuildSizeCounter(ns_fetch_serializers)
   BuildTypedByte_ArrayBuffer(ns_fetch_serializers)
   BuildByteArrayBuffer(ns_fetch_serializers)
   BuildTypeRegister(ns_fetch_serializers)
   BuildSerializableException(ns_fetch_serializers)
  */
  
//TODO: Dep problems
//  fetch::byte_array::BuildTokenizer(ns_fetch_byte_array);  
//  fetch::byte_array::BuildToken(ns_fetch_byte_array);


//  fetch::json::BuildJSONDocument(ns_fetch_json);
//  fetch::json::BuildUnrecognisedJSONSymbolException(ns_fetch_json);
  
/*
   BuildKeyValueSet(ns_fetch_http)
   BuildHTTPConnection(ns_fetch_http)
   BuildHTTPServer(ns_fetch_http)
   BuildSession(ns_fetch_http)
   BuildAbstractHTTPServer(ns_fetch_http)
   BuildHTTPResponse(ns_fetch_http)
   BuildHTTPConnectionManager(ns_fetch_http)
   BuildHTTPModule(ns_fetch_http)
   BuildHTTPRequest(ns_fetch_http)
   BuildAbstractHTTPConnection(ns_fetch_http)
   BuildRoute(ns_fetch_http)
*/
  
  fetch::containers::BuildVector<int>("VectorInt",ns_fetch_containers);
  // TODO: Add more types

/*
// Service
   BuildPacker(ns_fetch_service_details)
   BuildAbstractCallable(ns_fetch_service)
   BuildProtocol(ns_fetch_service)
   BuildPromiseImplementation(ns_fetch_service_details)
   BuildPromise(ns_fetch_service)
   BuildServiceServer(ns_fetch_service)
   BuildFunction(ns_fetch_service)
   BuildInvoke(ns_fetch_service)
   BuildUnrollArguments(ns_fetch_service)
   BuildFunction(ns_fetch_service)
   BuildServiceClientInterface(ns_fetch_service)
   BuildFeedSubscriptionManager(ns_fetch_service)
   BuildAbstractPublicationFeed(ns_fetch_service)
   BuildHasPublicationFeed(ns_fetch_service)
   BuildCallableClassMember(ns_fetch_service)
   BuildInvoke(ns_fetch_service)
   BuildUnrollArguments(ns_fetch_service)
   BuildServiceServerInterface(ns_fetch_service)
   BuildServiceClient(ns_fetch_service)
*/
  fetch::random::BuildLaggedFibonacciGenerator<418, 1279 > ( "LaggedFibonacciGenerator",ns_fetch_random);
  fetch::random::BuildLinearCongruentialGenerator(ns_fetch_random);
  fetch::random::BuildBitMask<uint64_t, 12, true >("BitMask", ns_fetch_random);

  fetch::random::BuildBitGenerator< fetch::random::LaggedFibonacciGenerator<>, 12, true >("BitGenerator",ns_fetch_random);
  

   /*
     // PROTOCOLS
   BuildChainController(ns_fetch_protocols)
   BuildSwarmController(ns_fetch_protocols)
   BuildSwarmProtocol(ns_fetch_protocols)
   BuildSharedNodeDetails(ns_fetch_protocols)
   BuildChainManager(ns_fetch_protocols)
   BuildChainKeeperController(ns_fetch_protocols)
   BuildChainKeeperProtocol(ns_fetch_protocols)
   BuildTransactionManager(ns_fetch_protocols)
*/
//   BuildRegisterInfo(ns_fetch_vectorize)
//   BuildBruteForceOptimiser(ns_fetch_optimisation)
//   BuildAbstractSpinGlassSolver(ns_fetch_optimisers)

  fetch::optimisers::BuildBinaryProblem(ns_fetch_optimisers);
  fetch::optimisers::BuildReferenceAnnealer(ns_fetch_optimisers);
  fetch::optimisers::BuildSparseAnnealer(ns_fetch_optimisers);

}
