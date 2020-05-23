#ifndef INCLUDED_VIDEO_WRITER_HPP
#define INCLUDED_VIDEO_WRITER_HPP

#include <string>
#include <glad/glad.h>
#include <opencv2/opencv.hpp>

#include "texture.hpp"

#define DEFAULT_CODEC cv::VideoWriter::fourcc('X', 'V', 'I', 'D')

/*動画コーデック例*/
//#define DEFAULT_CODEC cv::VideoWriter::fourcc('P','I','M','1')	//MPEG-1
//#define DEFAULT_CODEC cv::VideoWriter::fourcc('M','P','G','4')	//MPEG-4
//#define DEFAULT_CODEC cv::VideoWriter::fourcc('M','P','4','2')	//MPEG-4.2
//#define DEFAULT_CODEC cv::VideoWriter::fourcc('D','I','V','X')	//DivX
//#define DEFAULT_CODEC cv::VideoWriter::fourcc('D','X','5','0')	//DivX ver 5.0
//#define DEFAULT_CODEC cv::VideoWriter::fourcc('X','V','I','D')	//Xvid
//#define DEFAULT_CODEC cv::VideoWriter::fourcc('U','2','6','3')	//H.263
//#define DEFAULT_CODEC cv::VideoWriter::fourcc('H','2','6','4')	//H.264
//#define DEFAULT_CODEC cv::VideoWriter::fourcc('F','L','V','1')	//FLV1
//#define DEFAULT_CODEC cv::VideoWriter::fourcc('M','J','P','G')	//Motion JPEG
//#define DEFAULT_CODEC 0							//非圧縮
//#define DEFAULT_CODEC -1							//選択

namespace nekolib {
  namespace renderer {
    class VideoWriter {
    private:
      cv::Mat capture_image_;
      cv::VideoWriter video_writer_;
      nekolib::renderer::Texture tex_;
      int inter_polation_;

    public:
      // ctor, dtor
      VideoWriter() : inter_polation_(cv::INTER_LINEAR) {}
      VideoWriter(const char* fn, int width, int height,
		  double fps = 60.0, int codec = DEFAULT_CODEC) {
	set_output(fn, width, height, fps, codec);
      }
      ~VideoWriter() = default;

      // copy NG
      VideoWriter(const VideoWriter&) = delete;
      VideoWriter& operator=(const VideoWriter&) = delete;

      // move OK
      VideoWriter(VideoWriter&&) = default;
      VideoWriter& operator=(VideoWriter&&) = default;

      // 出力先設定
      void set_output(const char* fn, int width, int height,
		      double fps = 60.0, int codec = DEFAULT_CODEC) {
	video_writer_.open(fn, codec, fps, cv::Size(width, height), true);
    
	if (video_writer_.isOpened()) {
	  fprintf(stderr, "video capture file %s opened.\n", fn);
	}

	capture_image_ = cv::Mat(cv::Size(width, height), CV_8UC3);
	tex_ = nekolib::renderer::Texture::create(capture_image_.cols, capture_image_.rows,
						  nekolib::renderer::TextureFormat::RGB8);
      }

      // 現在のframebufferの中身を書き込む
      bool write() {
	if (video_writer_.isOpened()) {
	  tex_.bind(0);
	  glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0,
			   capture_image_.cols, capture_image_.rows, 0);
	  glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, capture_image_.data);

	  cvtColor(capture_image_, capture_image_, cv::ColorConversionCodes::COLOR_RGB2BGR);
	  flip(capture_image_, capture_image_, 0);
	  video_writer_ << capture_image_;
	  return true;
	} else {
	  return false;
	}
      }

      // 指定されたtextureの中身を書き込む
      bool write(nekolib::renderer::Texture tex) {
	if (video_writer_.isOpened()) {
	  tex.bind(0);
	  if (tex.width() == capture_image_.cols && tex.height() == capture_image_.rows) {
	    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, capture_image_.data);
	  } else {
	    cv::Mat temp(cv::Size(tex.width(), tex.height()), CV_8UC3);
	    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, temp.data);
	    cv::resize(temp, capture_image_, capture_image_.size(), 0.0, 0.0, inter_polation_);
	  }
	  cvtColor(capture_image_, capture_image_, cv::ColorConversionCodes::COLOR_RGB2BGR);
	  flip(capture_image_, capture_image_, 0);
	  video_writer_ << capture_image_;
	  return true;
	} else {
	  return false;
	}
      }

      void set_inter_polation(int ip) {
	inter_polation_ = ip;
      }
    };
  }
}

#endif // INCLUDED_VIDEO_WRITER_HPP
