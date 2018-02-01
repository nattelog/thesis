int file_read_count = 0;

// this function is called on each network request to read a file
// the req argument is the object containing all relevant information regarding
// the request
void read_file(req_t* req) {
  // read_file_from_disc yields control and let other
  file_t* file = read_file_from_disk(req->path_to_file);
  file_read_count++;
  req->send_file(file);
}
