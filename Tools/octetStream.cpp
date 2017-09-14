// (C) 2017 University of Bristol. See License.txt


#include <fcntl.h>
#include <sodium.h>

#include "octetStream.h"
#include <string.h>
#include "Networking/sockets.h"
#include "Tools/sha1.h"
#include "Exceptions/Exceptions.h"
#include "Networking/data.h"

void octetStream::assign(const octetStream& os)
{
  if (os.len>=mxlen)
    {
      if (data)
        delete[] data;
      mxlen=os.mxlen;  
      data=new octet[mxlen];
    }
  len=os.len;
  memcpy(data,os.data,len*sizeof(octet));
  ptr=os.ptr;
}


octetStream::octetStream(int maxlen)
{
  mxlen=maxlen; len=0; ptr=0;
  data=new octet[mxlen];
}


octetStream::octetStream(const octetStream& os)
{
  mxlen=os.mxlen;
  len=os.len;
  data=new octet[mxlen];
  memcpy(data,os.data,len*sizeof(octet));
  ptr=os.ptr;
}


void octetStream::hash(octetStream& output) const
{
  crypto_generichash(output.data, crypto_generichash_BYTES_MIN, data, len, NULL, 0);
  output.len=crypto_generichash_BYTES_MIN;
}


octetStream octetStream::hash() const
{
  octetStream h(crypto_generichash_BYTES_MIN);
  hash(h);
  return h;
}


bigint octetStream::check_sum(int req_bytes) const
{
  unsigned char hash[req_bytes];
  crypto_generichash(hash, req_bytes, data, len, NULL, 0);

  bigint ans;
  bigintFromBytes(ans,hash,req_bytes);
  // cout << ans << "\n";
  return ans;
}


bool octetStream::equals(const octetStream& a) const
{
  if (len!=a.len) { return false; }
  for (int i=0; i<len; i++)
    { if (data[i]!=a.data[i]) { return false; } }
  return true;
}


void octetStream::append_random(int num)
{
  resize(len+num);
  randombytes_buf(data+len, num);
  len+=num;
}


void octetStream::concat(const octetStream& os)
{
  resize(len+os.len);
  memcpy(data+len,os.data,os.len*sizeof(octet));
  len+=os.len;
}


void octetStream::store_bytes(octet* x, const int l)
{
  resize(len+4+l); 
  encode_length(data+len,l); len+=4;
  memcpy(data+len,x,l*sizeof(octet));
  len+=l;
}

void octetStream::get_bytes(octet* ans, int& length)
{
  length=decode_length(data+ptr); ptr+=4;
  memcpy(ans,data+ptr,length*sizeof(octet));
  ptr+=length;
}

void octetStream::store(unsigned int l)
{
  resize(len+4);
  encode_length(data+len,l); 
  len+=4;
}

void octetStream::store(int l)
{
  resize(len+4);
  INT_TO_BYTES(data+len,l);
  len+=4;
}


void octetStream::get(unsigned int& l)
{
  l=decode_length(data+ptr);
  ptr+=4;
}

void octetStream::get(int& l)
{
  l=BYTES_TO_INT(data+ptr);
  ptr+=4;
}


void octetStream::store(const bigint& x)
{
  int num=numBytes(x);
  resize(len+num+5);

  (data+len)[0]=0;
  if (x<0) { (data+len)[0]=1; }
  len++;

  encode_length(data+len,num); len+=4;
  bytesFromBigint(data+len,x,num);
  len+=num;
}


void octetStream::get(bigint& ans)
{
  int sign=(data+ptr)[0];
  if (sign!=0 && sign!=1) { throw bad_value(); }
  ptr++;

  long length=decode_length(data+ptr); ptr+=4;

  ans=0;
  if (length!=0)
    { bigintFromBytes(ans, data+ptr, length);
      ptr+=length;
      if (sign==1) { ans=-ans; }
    }
}

// Construct the ciphertext as `crypto_secretbox(pt, counter||random)`
void octetStream::encrypt_sequence(const octet* key, uint64_t counter)
{
  octet nonce[crypto_secretbox_NONCEBYTES];
  int i;
  int message_len_bytes = len;
  randombytes_buf(nonce, sizeof nonce);
  if(counter == UINT64_MAX) {
      throw Processor_Error("Encryption would overflow counter. Too many messages.");
  } else {
      counter++;
  }
  for(i=0; i<8; i++) {
      nonce[i] = uint8_t ((counter >> (8*i)) & 0xFF);
  }

  resize(len + crypto_secretbox_MACBYTES + crypto_secretbox_NONCEBYTES);

  // Encrypt data in-place
  crypto_secretbox_easy(data, data, message_len_bytes, nonce, key);
  // Adjust length to account for MAC, then append nonce
  len += crypto_secretbox_MACBYTES;
  append(nonce, sizeof nonce);
}

void octetStream::decrypt_sequence(const octet* key, uint64_t counter)
{
  int ciphertext_len = len - crypto_box_NONCEBYTES;
  const octet *nonce = data + ciphertext_len;
  int i;
  uint64_t recvCounter=0;
  // Numbers are typically 24U + 16U so cast to int is safe.
  if (len < (int)(crypto_box_NONCEBYTES + crypto_secretbox_MACBYTES))
  {
    throw Processor_Error("Cannot decrypt octetStream: ciphertext too short");
  }
  for(i=7; i>=0; i--) {
      recvCounter |= (uint64_t) *(nonce + i);
      recvCounter = recvCounter << (i*8);
  }
  if(recvCounter != counter + 1) {
      throw Processor_Error("Incorrect counter on stream.  Possible MITM.");
  }
  if (crypto_secretbox_open_easy(data, data, ciphertext_len, nonce, key) != 0)
  {
    throw Processor_Error("octetStream decryption failed!");
  }
  rewind_write_head(crypto_box_NONCEBYTES + crypto_secretbox_MACBYTES);
}

void octetStream::encrypt(const octet* key)
{
  octet nonce[crypto_secretbox_NONCEBYTES];
  randombytes_buf(nonce, sizeof nonce);
  int message_len_bytes = len;
  resize(len + crypto_secretbox_MACBYTES + crypto_secretbox_NONCEBYTES);

  // Encrypt data in-place
  crypto_secretbox_easy(data, data, message_len_bytes, nonce, key);
  // Adjust length to account for MAC, then append nonce
  len += crypto_secretbox_MACBYTES;
  append(nonce, sizeof nonce);
}

void octetStream::decrypt(const octet* key)
{
  int ciphertext_len = len - crypto_box_NONCEBYTES;
  // Numbers are typically 24U + 16U so cast to int is safe.
  if (len < (int)(crypto_box_NONCEBYTES + crypto_secretbox_MACBYTES))
  {
    throw Processor_Error("Cannot decrypt octetStream: ciphertext too short");
  }
  if (crypto_secretbox_open_easy(data, data, ciphertext_len, data + ciphertext_len, key) != 0)
  {
    throw Processor_Error("octetStream decryption failed!");
  }
  rewind_write_head(crypto_box_NONCEBYTES + crypto_secretbox_MACBYTES);
}

ostream& operator<<(ostream& s,const octetStream& o)
{
  for (int i=0; i<o.len; i++)
    { int t0=o.data[i]&15;
      int t1=o.data[i]>>4;
      s << hex << t1 << t0 << dec;
    }
  return s;
}




