#include "Index.h"

#include "bparse.h"

namespace xiv 
{
namespace dat 
{
   struct IndexBlockRecord
   {
      uint32_t offset;
      uint32_t size;
      SqPackBlockHash blockHash;
   };

   struct IndexHashTableEntry
   {
      uint32_t filenameHash;
      uint32_t dirHash;
      uint32_t datOffset;
      uint32_t padding;
   };
}
}

namespace xiv 
{
namespace utils
{
namespace bparse
{
   template <>
   inline void reorder<xiv::dat::IndexBlockRecord>(xiv::dat::IndexBlockRecord& i_struct)
   {
      xiv::utils::bparse::reorder(i_struct.offset);
      xiv::utils::bparse::reorder(i_struct.size);
      xiv::utils::bparse::reorder(i_struct.blockHash);
   }

   template <>
   inline void reorder<xiv::dat::IndexHashTableEntry>(xiv::dat::IndexHashTableEntry& i_struct)
   {
      xiv::utils::bparse::reorder(i_struct.filenameHash);
      xiv::utils::bparse::reorder(i_struct.dirHash);
      xiv::utils::bparse::reorder(i_struct.datOffset);
      xiv::utils::bparse::reorder(i_struct.padding);
   }
}
}
};

using xiv::utils::bparse::extract;

namespace xiv
{
namespace dat
{

Index::Index(const boost::filesystem::path& path) :
   SqPack( path )
{
   if( !m_handle )
      throw new std::runtime_error( "Failed to load Index at " + path.string() );

   // Hash Table record
   auto hashTableBlockRecord = extract<IndexBlockRecord>( m_handle );
   isIndexBlockValid( hashTableBlockRecord );

   // Save the posin the stream to go back to it later on
   auto pos = m_handle.tellg();

   // Seek to the pos of the hash table in the file
   m_handle.seekg( hashTableBlockRecord.offset );

   // Preallocate and extract the index_hash_table_entries
   std::vector<IndexHashTableEntry> indexHashTableEntries;
   extract<IndexHashTableEntry>( m_handle, hashTableBlockRecord.size / sizeof( IndexHashTableEntry ),
                                 indexHashTableEntries );

   // Feed the correct entry in the HashTable for each index_hash_table_entry
   for( auto& indexHashTableEntry : indexHashTableEntries )
   {
      auto& hashTableEntry = m_hashTable[indexHashTableEntry.dirHash][indexHashTableEntry.filenameHash];
      // The dat number is found in the offset, last four bits
      hashTableEntry.datNum = ( indexHashTableEntry.datOffset & 0xF ) / 0x2;
      // The offset in the dat file, needs to strip the dat number indicator
      hashTableEntry.datOffset = ( indexHashTableEntry.datOffset & 0xFFFFFFF0 ) * 0x08;
      hashTableEntry.dirHash = indexHashTableEntry.dirHash;
      hashTableEntry.filenameHash = indexHashTableEntry.filenameHash;
   }

   // Come back to where we were before reading the HashTable
   m_handle.seekg( pos );

   // Dat Count
   m_datCount = extract<uint32_t>( m_handle, "dat_count" );

   // Free List
   isIndexBlockValid( extract<IndexBlockRecord>( m_handle ) );

   // Dir Hash Table
   isIndexBlockValid( extract<IndexBlockRecord>( m_handle ) );
}

Index::~Index()
{
}

uint32_t Index::getDatCount() const
{
   return m_datCount;
}

const Index::HashTable& Index::getHashTable() const
{
   return m_hashTable;
}

bool Index::doesFileExist( uint32_t dir_hash, uint32_t filename_hash ) const
{
   auto dir_it = getHashTable().find( dir_hash );
   if( dir_it != getHashTable().end() )
   {
      return ( dir_it->second.find( filename_hash ) != dir_it->second.end() );
   }
   return false;
}

bool Index::doesDirExist( uint32_t dir_hash ) const
{
   return ( getHashTable().find( dir_hash ) != getHashTable().end() );
}

const Index::DirHashTable& Index::getDirHashTable( uint32_t dir_hash ) const
{
   auto dir_it = getHashTable().find( dir_hash );
   if( dir_it == getHashTable().end() )
   {
      throw std::runtime_error( "dirHash not found" );
   }
   else
   {
      return dir_it->second;
   }
}

const Index::HashTableEntry& Index::getHashTableEntry( uint32_t dir_hash, uint32_t filename_hash ) const
{
   auto& dirHashTable = getDirHashTable( dir_hash );
   auto file_it = dirHashTable.find( filename_hash );
   if( file_it == dirHashTable.end() )
   {
      throw std::runtime_error( "filenameHash not found" );
   }
   else
   {
      return file_it->second;
   }
}

void Index::isIndexBlockValid( const IndexBlockRecord& i_index_block_record )
{
   isBlockValid( i_index_block_record.offset, i_index_block_record.size, i_index_block_record.blockHash );
}

}
}
