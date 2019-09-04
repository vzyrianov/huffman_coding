#include <map>
#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <functional>
#include <string>
#include <limits>
#include <bitset>

struct bit_encoding
{
   int length;
   uint64_t value;

   bool operator<(const bit_encoding& lhs) const
   {
      return std::tie(length, value) < std::tie(lhs.length, lhs.value);
   }

   bit_encoding append_1() const
   {
      return { length + 1, (value << 1) + 1 };
   }

   bit_encoding append_0() const
   {
      return { length + 1, value << 1 };
   }

   void take_from(bit_encoding& other)
   {
      length += 1;
      value = value << 1;

      uint64_t x = 1;

      value += ((other.value & (x << (other.length - 1)))) != 0;

      other.value = other.value & ~(x << (other.length - 1));
      --other.length;
   }
};

struct huffman_node
{
   char value;
   bool isLeaf;

   int count;

   huffman_node* left;
   huffman_node* right;

   void print(const std::string& prefix) const
   {
      if (isLeaf)
      {
         std::cout << value << " " << prefix << std::endl;
      }
      else
      {
         left->print(prefix + "0");

         right->print(prefix + "1");
      }
   }

   void build_encoding(std::map<char, bit_encoding>& encoding_to_build, bit_encoding current) const
   {
      if (isLeaf)
      {
         encoding_to_build[value] = current;
      }
      else
      {
         left->build_encoding(encoding_to_build, current.append_0());

         right->build_encoding(encoding_to_build, current.append_1());
      }
   }

   void build_decoding(std::map<bit_encoding, char>& encoding_to_build, bit_encoding current) const
   {
      if (isLeaf)
      {
         encoding_to_build[current] = value;
      }
      else
      {
         left->build_decoding(encoding_to_build, current.append_0());

         right->build_decoding(encoding_to_build, current.append_1());
      }
   }
};

const char BACK = '\a';
const char ESCAPE = '\\';

void write_huffman(std::ofstream& output, huffman_node* root)
{
   if (root->isLeaf) {
      if(root->value == BACK)
      {
         output.write(&ESCAPE, 1);
         output.write(&root->value, 1);
         output.write(&BACK, 1);
      }
      else if(root->value == ESCAPE)
      {
         output.write(&ESCAPE, 1);
         output.write(&ESCAPE, 1);
         output.write(&BACK, 1);
      }
      else
      {
         output.write(&root->value, 1);
         output.write(&BACK, 1);
      }
   }
   else
   {
      output.write(&root->value, 1);
      write_huffman(output, root->left);
      write_huffman(output, root->right);
      output.write(&BACK, 1);
   }
}

huffman_node* read_huffman(std::ifstream& input)
{
   char current_token;
   char next_token;

   input.read(&current_token, 1);
   next_token = input.peek();

   if(current_token == ESCAPE)
   {
      input.read(&current_token, 1);
      next_token = input.peek();
   }

   bool isLeaf = next_token == BACK;

   if(isLeaf)
   {
      huffman_node* result = new huffman_node{ current_token, isLeaf, 0, nullptr, nullptr };

      input.read(&current_token, 1);

      return result;
   }
   else
   {
      huffman_node* left = read_huffman(input);
      huffman_node* right = read_huffman(input);
      huffman_node* result = new huffman_node{ current_token, isLeaf, 0, left, right };

      input.read(&current_token, 1);

      return result;
   }
}
 
struct bit_encoding_writer
{
   std::ofstream* stream;
   bit_encoding buffer;

   bit_encoding_writer(std::ofstream* s)
   {
      stream = s;
      buffer = {0, 0};
   };

   void write(bit_encoding bits)
   {
      if(bits.length <= (64 - buffer.length))
      {
         buffer.value = buffer.value << bits.length;
         buffer.value += bits.value;
         buffer.length += bits.length;

         if (buffer.length == 64)
            write_buffer();
      } else
      {
         if(bits.length == 64)
         {
            write_buffer();
            write(bits);
         }
         else
         {
            const int amount_empty = 64 - buffer.length;
            const uint64_t copy = bits.value >> bits.length - amount_empty;
            write({amount_empty, copy});
            write({ bits.length - amount_empty, bits.value - (copy << bits.length - amount_empty) });
         }
      }
   }

   void close()
   {
      while (buffer.length != 64)
      {
         buffer = buffer.append_0();
      }

      write_buffer();

      stream->flush();
      stream->close();
   }

private:
   void write_buffer()
   {
      stream->write(reinterpret_cast<char *>(&buffer.value), sizeof(buffer.value));
      buffer.length = 0;
   }
};


huffman_node* build_huffman(std::ifstream& file)
{
   //Calculate Frequencies
   std::map<char, int> frequencies;
   char currentChar;

   while (file.get(currentChar))
   {
      frequencies[currentChar] += 1;
   }
   frequencies['\0'] += 1; //Add file end encoding
   frequencies[ESCAPE] += 1; //Add escape character encoding

   //Create queue
   struct comparator
   {
      bool operator()(huffman_node* lhs, huffman_node* rhs) const
      {
         return lhs->count > rhs->count;
      }
   };

   std::priority_queue<huffman_node*, std::vector<huffman_node*>, comparator> queue;

   for (auto& kv : frequencies)
   {
      queue.push(new huffman_node{kv.first, true, kv.second, nullptr, nullptr});
   }

   //Build huffman tree
   while (queue.size() > 1)
   {
      huffman_node* first = queue.top();
      queue.pop();

      huffman_node* second = queue.top();
      queue.pop();

      huffman_node* branch = new huffman_node {
         '\0',
         false,
         first->count + second->count,
         (first->count > second->count) ? second : first,
         (first->count > second->count) ? first : second
      };

      queue.push(branch);
   }

   return queue.top();
}

void encode(std::ifstream& to_encode, std::map<char, bit_encoding> encoding, std::ofstream* stream)
{
   bit_encoding_writer writer(stream);

   char c;
   while(to_encode.get(c))
   {
      if(c == '\0' || c == ESCAPE)
      {
         writer.write(encoding[ESCAPE]);
      }

      writer.write(encoding[c]);
   }

   writer.write(encoding['\0']);

   writer.close();

}

void decode(std::ifstream& input, std::map<bit_encoding, char> decoding, std::ofstream& stream)
{
   bit_encoding read_in = {0, 0};
   bit_encoding matching = { 0, 0 };
   std::string result;

   input.read(reinterpret_cast<char*>(&read_in.value), sizeof(read_in.value));
   read_in.length = 64;

   bool currently_escaped = false;

   char c;
   while (true)
   {
      while (decoding.find(matching) == decoding.end())
      {
         if (read_in.length == 0)
         {
            input.read(reinterpret_cast<char*>(&read_in.value), sizeof(read_in.value));
            read_in.length = 64;
         }

         matching.take_from(read_in);
      }
      c = decoding[matching];

      if((c == ESCAPE) && (!currently_escaped))
      {
         currently_escaped = true;
      }
      else
      {
         if (c == '\0')
            break;

         currently_escaped = false;

         stream << c;
      }

      matching = { 0, 0 };
   }
}

int main(int argc, char* argv[])
{
   if(argc != 4)
   {
      std::cout << "Incorrect number of parameters!";
      return 1;
   }
   if(std::string(argv[1]) == "e")
   {
      //Build huffman tree
      std::ifstream file(argv[2]);
      huffman_node* tree = build_huffman(file);
      file.close();

      //Write huffman tree out to file
      std::ofstream output(argv[3], std::ios_base::binary);
      write_huffman(output, tree);

      //Create encodings
      std::map<char, bit_encoding> encoding;
      tree->build_encoding(encoding, bit_encoding());

      //read through file again and encode it
      std::ifstream file2(argv[2]);
      encode(file2, encoding, &output);
      file2.close();

      output.flush();
      output.close();
   }
   else if (std::string(argv[1]) == "d")
   {
      //Read in tree
      std::ifstream input(argv[2], std::ios_base::binary);
      huffman_node* tree_copy = read_huffman(input);
      //tree_copy->print("");

      //Build decoding

      std::map<bit_encoding, char> decoding;
      tree_copy->build_decoding(decoding, bit_encoding());


      std::ofstream stream(argv[3]);
      decode(input, decoding, stream);
   }

   return 0;
}
