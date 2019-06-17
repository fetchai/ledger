#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include <memory>
#include <utility>
#include <vector>
#include <fstream>

#include "math/base_types.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/dataloader.hpp"


namespace fetch {
  namespace ml {

    std::pair<math::SizeType, math::SizeType> count_rows_cols(std::string filename, bool transpose=false)
    {
      // find number of rows and columns in the file
      std::ifstream file(filename);
      std::string   buf;
      std::string           delimiter = ",";
      ulong                 pos;
      math::SizeType row{0};
      math::SizeType col{0};
      while (std::getline(file, buf, '\n'))
      {
        while (row == 0 && ((pos = buf.find(delimiter)) != std::string::npos))
        {
          buf.erase(0, pos + delimiter.length());
          ++col;
        }
        ++row;
      }
      if (transpose)
      {
        return std::make_pair(col+1, row);
      } else
      {
        return std::make_pair(row, col+1);
      }
    }

    template <typename LabelType, typename DataType>
    class CommodityDataLoader : DataLoader<LabelType, DataType>
    {

    public:
      using ReturnType = std::pair<LabelType, std::vector<DataType>>;
      using SizeType = math::SizeType;

      CommodityDataLoader(bool random_mode)
          : DataLoader<LabelType, DataType>(random_mode)
      {}
      ~CommodityDataLoader() = default;
      ReturnType GetNext() override
      {
        GetAtIndex(static_cast<SizeType>(cursor_++));
        return buffer_;
      }

//      ReturnType GetRandom()  // todo: isn't this obsolete?
//      {
//        GetAtIndex((SizeType)rand() % Size(), buffer_);
//        return buffer_;
//      }

//      SizeType Size() const   = 0; // todo: AddData should store size of buffer, number of rows,

      virtual SizeType Size() const
      {
        // MNIST files store the size as uint32_t but Dataloader interface require uint64_t
        return static_cast<SizeType>(size_);
      }

      virtual bool IsDone() const
      {
        return cursor_ >= size_;
      }

      virtual void Reset()
      {
        cursor_ = 0;
      }

      // todo: handle labels

      void AddData(std::string xfilename, std::string yfilename)
      {
//
//
//        // find number of rows and columns in the file
//        std::string           delimiter = ",";
//        ulong                 pos;
//        SizeType row{0};
//        SizeType col{0};
//        while (std::getline(file, buf, '\n'))
//        {
//          while (row == 0 && ((pos = buf.find(delimiter)) != std::string::npos))
//          {
//            buf.erase(0, pos + delimiter.length());
//            ++col;
//          }
//          ++row;
//        }

        std::pair<SizeType , SizeType > xshape = count_rows_cols(xfilename);
        std::pair<SizeType , SizeType > yshape = count_rows_cols(yfilename);
        SizeType row = xshape.first;
        SizeType col = xshape.second;

        data_.Reshape({row - rows_to_skip_, col - cols_to_skip_});
        labels_.Reshape({yshape.first - rows_to_skip_, yshape.second - cols_to_skip_});
        assert(xshape.first == yshape.first);
        // save the number of rows in the data
        size_ = row - rows_to_skip_;
        // set up the buffer
        buffer_.second = std::vector<DataType>(static_cast<int>(col - cols_to_skip_));
        buffer_.first = LabelType(yshape.second - cols_to_skip_);

        // todo: read csv in separate function?

        // read csv data into buffer_ array
        std::ifstream file(xfilename);
        std::string   buf;
//        file.clear();
//        file.seekg(0, std::ios::beg);
        char delimiter = ',';
        std::string field_value;


        for(SizeType i=0; i<rows_to_skip_; i++)
        {
          std::getline(file, buf, '\n');
        }

        row = 0;
        while (std::getline(file, buf, '\n'))
//          while( file.peek() != EOF )
        {
          col = 0;
          std::stringstream ss(buf);
          for ( SizeType i = 0; i < cols_to_skip_; i++)
          {
            std::getline(ss, field_value, delimiter);
//            pos = buf.find(delimiter);
//            buf.erase(0, pos + delimiter.length());
          }

//          while ((pos = buf.find(delimiter)) != std::string::npos)
          while (std::getline(ss, field_value, delimiter))
          {
            data_(row, col) = static_cast<DataType>(stod(field_value));
//            buf.erase(0, pos + delimiter.length());
            ++col;
          }
//          data_(row, col) = static_cast<DataType>(std::stod(buf));
          ++row;
        }

        file.close();

        file.open(yfilename);
        for(SizeType i=0; i<rows_to_skip_; i++)
        {
          std::getline(file, buf, '\n');
        }

        row = 0;
        while (std::getline(file, buf, '\n'))
        {
          col = 0;
          std::stringstream ss(buf);
          for ( SizeType i = 0; i < cols_to_skip_; i++)
          {
            std::getline(ss, field_value, delimiter);
          }

          while (std::getline(ss, field_value, delimiter))
          {
            labels_(row, col) = static_cast<DataType>(stod(field_value));
            ++col;
          }
          ++row;
        }
      }

    private:
      bool random_mode_ = false;
      fetch::math::Tensor<DataType> data_;   // features, n_data
      fetch::math::Tensor<DataType> labels_;   // features, n_data

//      unsigned char **data_;
//      unsigned char * labels_;
      SizeType rows_to_skip_ = 1;
      SizeType cols_to_skip_ = 1;
      ReturnType buffer_;
      SizeType cursor_ = 0;
      SizeType size_ = 0;

      void GetAtIndex(SizeType index)
      {
        SizeType i{0};
        auto     it = buffer_.second.at(0).begin();
        while (it.is_valid())
        {
          *it = data_(index, i);
          ++i;
          ++it;
        }  // todo: foreach

        buffer_.first.Fill(DataType{0});
        buffer_.first(labels_[index], 0) = DataType{1.0};
      }
    };

  }  // namespace ml
}  // namespace fetch
