# Readme
# P2P-File-Transfer
Final Assignment of COMP2017

## Part 1 description: A program to use verify file check data file by chunks.
 -  **bpkg load and file check or create file**
        bpkg_load and bpkg_file_check were the first two steps of package management. Passing deposited the data information into bpkg_obj to be pre-work, the struct will useful for us to verify the data file. And in bpkg_file_check, it will check the data that corresponds to .bpkg file existed and check the base information of the file, like size, if didn't exist, we need to create a file with the same name as the filename in bpkg_obj. This is all really for the fetch command in Part 2.

 -  **creating merkletree and apply hash into the nodes**
        This part are going to build a merkletree in the process of bpkg_load, the reason why it should build when bpkg be load is that every single step, whether check hash, find hash, compute hash or any process about chunks will related to the tree.
        We use bpkg_obj as the parameter, then create the non_leaf node (build_merkle_tree_structure), we need to create obj->nhashes nodes include the root. Then we use assign_hashes_bfs to apply the expected_hash into nodes.
        Then the basic merkletree is finished without the chunks hash we calculate.

 -  **compute the hash from chunks**
        When we need to get min complete hashes, it will call function bpkg_get_completed_chunks, and calculate hash by the data chunks. Then use add_computed_to_tree to determine if the compute_hash is the same as the expected hash in leaf node.
        The method here is to travel all the non_leaf nodes in bfs, because it will make the visit in line by line, not the node and its child. So when we meet the first node with is_leaf, this means we are coming to the bottom line of the node. And we can just add all the compute hash into node if thier expected hash the same. It can avoid the situation that the compute qry doesn't match the amount of leaf nodes. Ensure all the compute hash is useful.
        Until here, the process of generating merkletree is over.

 -  **find the hash of the top level of node that its leaf nodes all matched.**
        In bpkg_get_min_completed_hashesn, we use get completed_chunk to add all the chunks from the file to tree, and create a qry to  save the hash,it work by tracing all the leaf_nodes that will connect to the current non_leaf node. Determine their expected hash and compute hash. If all the same means the current hash must be correct. So we can add into qry. If the leaf_nodes have different, or the same, search=1 will lock all the child nodes of the current node. So we can avoid visiting again whether we use any method to travel the tree. This feature ensures that all the non_leaf nodes and chunks will only check once. And save to qry.

 -  **find all the leaf node that will connect to a node**
        This is what bpkg_get_all_chunk_hashes_from_hash is trying to do, by a hash given, we will find the non-leaf node in find_node, by travelling all the nodes and comparing the hash with the expected hash. After that, use collect_chunk_hash to tracert to the bottom, which is the leaf node. The method is to travel all the left and right nodes of this given node. and check if the node is a leaf node, return hash to the query.


## Part 2 description: A program can be server and client in P2P file transfer.
 -  **config reading and basic scaffold of Btide**
        The program will load the config file, start a listen server thread, then start a while loop for the received command.
        In config.c we have a struct Config to save the directory char, int peers limit and port. The program just parses the file into memory and saves to Config, check the directory exists in the current dir. If not, we should create the folder. And it will check the max_peer is between 1-2028, the port is between 1024-65535.

 -  **Listen server**
        client_listen is a listen server, will process all the connection request and manage the received packet and sent back to the peer. We create a listen sock to use config ->port. and accept the connection by conn_fd. If the server is running, the accept won't be offline. The first time connects will send a error2 to tell the server need to sent back a ACP pack when the connection is established. Also, we will response PNG,DSN,REQ packet and reply.

 -  **peer and package management**
        There is a peer structure and a single link list to save the peer. Each peer contain a sock and address. create_node will be called when we ensure the connection is established. set_max_peers decide the length of list. and disconnect is to delete the node find_peer_node is use ip_port to find is the peer has been recorded in the link list. 
        In the package, we have package sharing here, recived_ACK for checking the first-time connection successful, send_DSN will use when we disconnect，tell the server I'm leaving.test_and_cleanup_peers will call test_peer_connection, and send the server PNG and expect to receive POG to keep connected.

 -  **multi-pthreads client and CONNECT/DISCONNECT/PEERS command**
        Every time we connect, if the comment is valid，it will create a handle_connection thread, to process this peer/sock. It will ensure every connection is independent. Disconnect will use the input ip_port to travel the single link list and remove the node. For peer we are going to run test_and_cleanup_peer to sent PNG message by each node and remove the node without any respond.

 -  **ADDPACKAGE, REMPACKAGE, PACKAGES commands**
        ADDPACKAGE will direct our working dir into the config->directory. Then try to locate the .bpkg file. If the bpkg file exited, we load and use file_check to ensure the placeholder file exists (because what we will do is get the chunks from server and replace the place in the placeholder file, till the download is finished). After that, add the bpkg_obj into a obj_list struct that I define on the top of the file. It is used for saving the obj, we will use it in package management, also the management on transfer data. REMPACKAGE is easy, just travel the obj_list and remove the obj if there have obj->ident matched.
        In PACKAGES I am going to use bpkg_get_min_completed_hashes to check my local file is correct with the verify file(.bpkg).

 -  **FETCH, REQ and RSQ works**
        In command FETCH, we copy the command_content into ip_port, identifier, hash and offset(default 0) first. The second step is to use find_peer_node, we need the sock to connect to the server. Traveling the obj_list and find the chunks we need to download, then create a packet_list to receive one or N * RES packet, because the overlength data will sent back in serial times. 
        In sent_REQ_RES, write the file_offset, data_len,chunk_hash and identifier into packet.p1.data, and send to the server, wait for the RES packet. The received time should be the value of data_len divided by 2998(the max size of data in one packet), and select the next integer of the result. 
        After making all the packets into packet_list, we use load_package to load the data and write it into the file. Because we need to verify the identity and chunk hash when we send REQ, so the data we need to write in must be the obj-> filename. Just mappe the file in to memory and write the data to the file.


## Testcase situation Introduction:
   # All the files in the folder: testcases are only for testing, and the binary file(.data) are only for testing use.
    P1 Test1: Over hahes/chunks
    P1 Test2: hash in bpkg file be change
    P1 Test3: chunks in bpkg file be change
    P1 Test4: file name missing
    P1 Test5: file name over size
    P1 Test6: nhashes is zero
    P1 Test7: ident not found
    p1 Test8: Merkle_nchunks and nhashes difference more than one
    P1 Test9: Check the leaf node inside last non-leaf node
    P1 Test10: two chunks create tree

    P2 Test11: create directory (TEST CONFIG)
    P2 Test12: switch listen port by new config (TEST SERVER)
    P2 Test13: repeat connect to the same peer (TEST CONNECT)
    P2 Test14: data file broken (TEST ADDPACKAGE)
    P2 Test15: Add same package and Remove with 2 wrong command (TEST REMPACKAGE,PACKAGES)
    P2 Test16: loop of connect and disconnect and PEERS of multi address (TEST CONNECT,DISCONNECT,PEERS)

## Additional informaiton:
  # use command "make" in the terminal can run all the testing, and automatic clean the file