#include "monarch3_wrap.hh"

#include "M3DataInterface.hh"
//#include "M3Monarch.hh"

#include "logger.hh"
#include "psyllid_error.hh"

#include <cstring> // for strcmp

using namespace monarch3;
using namespace psyllid;

LOGGER( plog, "test_monarch3_write" );

int main( const int argc, const char** argv )
{
    if( argc < 2 || strcmp( argv[1], "-h" ) == 0 )
    {
        INFO( plog, "usage:\n"
            << "  test_monarch3_write [-h] <output egg file>\n"
            << "      -h: print this usage information" );
        return -1;
    }

    try
    {
        monarch_wrap_ptr t_mwp( new monarch_wrapper( argv[1] ) );
        //Monarch3* tWriteTest = Monarch3::OpenForWriting( argv[1] );

        header_wrap_ptr t_hwp( t_mwp->get_header() );
        //M3Header* tHeader = tWriteTest->GetHeader();
        t_hwp->header().SetFilename( argv[1] );
        t_hwp->header().SetRunDuration( 8675309 );
        t_hwp->header().SetTimestamp( "Stardate 33515" );
        t_hwp->header().SetDescription( "Bigger on the inside" );

        // number of samples in each stream
        unsigned tSSSamples = 10;
        unsigned tDSSamples = 5;
        unsigned tTSSamples = 5;
        unsigned tFlSSamples = 10;
        //unsigned tFlCompSamples = 5;

        INFO( plog, "Adding streams" );
        unsigned tSingleStreamNum = t_hwp->header().AddStream( "1-channel device", 500, tSSSamples, 1, 1, sDigitizedUS, 8, sBitsAlignedLeft );
        unsigned tDoubleStreamNum = t_hwp->header().AddStream( "2-channel device", 2, sInterleaved, 250, tDSSamples, 1, 2, sDigitizedUS, 16, sBitsAlignedLeft );
        unsigned tTripleStreamNum = t_hwp->header().AddStream( "3-channel device", 3, sSeparate, 100, tTSSamples, 1, 1, sDigitizedUS, 8, sBitsAlignedLeft );
        unsigned tFloatStreamNum = t_hwp->header().AddStream( "Floating-point device", 100, tFlSSamples, 1, 4, sAnalog, 8, sBitsAlignedLeft );
        // multi-channel multi-sample writing commented out until fixed
        //unsigned tFlCompStreamNum = t_hwp->header().AddStream( "Complex Floating-point device", 5, sInterleaved, 100, tFlCompSamples, 2, 8, sAnalog, 16 );

        //tWriteTest->WriteHeader();

        INFO( plog, "Wrote header:\n" << t_hwp->header() );

        INFO( plog, "Writing data" );

        // Stream 0
        stream_wrap_ptr t_swp0 = t_mwp->get_stream( tSingleStreamNum );
        //M3Stream* tSingleStream = tWriteTest->GetStream( tSingleStreamNum );
        byte_type* tSSData = t_swp0->stream().GetStreamRecord()->GetData();
        for( unsigned iSample = 0; iSample < tSSSamples; ++iSample )
        {
            tSSData[ iSample ] = 1;
        }
        if( ! t_swp0->stream().WriteRecord( true ) )
        {
            ERROR( plog, "Unable to write the record!" );
            //delete tWriteTest;
            return -1;
        }

        for( unsigned iSample = 0; iSample < tSSSamples; ++iSample )
        {
            tSSData[ iSample ] = 10;
        }
        if( ! t_swp0->stream().WriteRecord( false ) )
        {
            ERROR( plog, "Unable to write the record!" );
            //delete tWriteTest;
            return -1;
        }

        t_mwp->finish_stream( tSingleStreamNum );


        // Stream 1
        stream_wrap_ptr t_swp1 = t_mwp->get_stream( tDoubleStreamNum );
        //M3Stream* tDoubleStream = tWriteTest->GetStream( tDoubleStreamNum );
        M3DataWriter< uint16_t > tDSData0( t_swp1->stream().GetChannelRecord( 0 )->GetData(), 2, sDigitizedUS );
        M3DataWriter< uint16_t > tDSData1( t_swp1->stream().GetChannelRecord( 1 )->GetData(), 2, sDigitizedUS );
        for( unsigned iSample = 0; iSample < tDSSamples; ++iSample )
        {
            tDSData0.set_at( 1, iSample );
            tDSData1.set_at( 2, iSample );
        }
        if( ! t_swp1->stream().WriteRecord( true ) )
        {
            ERROR( plog, "Unable to write the record!" );
            //delete tWriteTest;
            return -1;
        }

        for( unsigned iSample = 0; iSample < tDSSamples; ++iSample )
        {
            tDSData0.set_at( 1000, iSample );
            tDSData1.set_at( 2000, iSample );
        }
        if( ! t_swp1->stream().WriteRecord( true ) )
        {
            ERROR( plog, "Unable to write the record!" );
            //delete tWriteTest;
            return -1;
        }

        for( unsigned iSample = 0; iSample < tDSSamples; ++iSample )
        {
            tDSData0.set_at( 10000, iSample );
            tDSData1.set_at( 20000, iSample );
        }
        if( ! t_swp1->stream().WriteRecord( false ) )
        {
            ERROR( plog, "Unable to write the record!" );
            //delete tWriteTest;
            return -1;
        }

        t_mwp->finish_stream( tDoubleStreamNum );


        // Stream 2
        stream_wrap_ptr t_swp2 = t_mwp->get_stream( tTripleStreamNum );
        //M3Stream* tTripleStream = tWriteTest->GetStream( tTripleStreamNum );
        byte_type* tTSData0 = t_swp2->stream().GetChannelRecord( 0 )->GetData();
        byte_type* tTSData1 = t_swp2->stream().GetChannelRecord( 1 )->GetData();
        byte_type* tTSData2 = t_swp2->stream().GetChannelRecord( 2 )->GetData();
        for( unsigned iSample = 0; iSample < tTSSamples; ++iSample )
        {
            tTSData0[ iSample ] = 1;
            tTSData1[ iSample ] = 2;
            tTSData2[ iSample ] = 3;
        }
        if( ! t_swp2->stream().WriteRecord( true ) )
        {
            ERROR( plog, "Unable to write the record!" );
            //delete tWriteTest;
            return -1;
        }

        for( unsigned iSample = 0; iSample < tTSSamples; ++iSample )
        {
            tTSData0[ iSample ] = 10;
            tTSData1[ iSample ] = 20;
            tTSData2[ iSample ] = 30;
        }
        if( ! t_swp2->stream().WriteRecord( false ) )
        {
            ERROR( plog, "Unable to write the record!" );
            //delete tWriteTest;
            return -1;
        }

        t_mwp->finish_stream( tTripleStreamNum );


        // Stream 3
        stream_wrap_ptr t_swp3 = t_mwp->get_stream( tFloatStreamNum );
        //M3Stream* tFloatStream = tWriteTest->GetStream( tFloatStreamNum );
        M3DataWriter< float > tFlSData( t_swp3->stream().GetChannelRecord( 0 )->GetData(), 4, sAnalog );
        for( unsigned iSample = 0; iSample < tFlSSamples; ++iSample )
        {
            tFlSData.set_at( 3.1415926535898, iSample );
        }
        if( ! t_swp3->stream().WriteRecord( true ) )
        {
            ERROR( plog, "Unable to write the record!" );
            //delete tWriteTest;
            return -1;
        }

        for( unsigned iSample = 0; iSample < tFlSSamples; ++iSample )
        {
            tFlSData.set_at( 2.71828182846, iSample );
        }
        if( ! t_swp3->stream().WriteRecord( true ) )
        {
            ERROR( plog, "Unable to write the record!" );
            //delete tWriteTest;
            return -1;
        }

        t_mwp->finish_stream( tFloatStreamNum );


        // Stream 4
        /*
        stream_wrap_ptr t_swp4 = t_mwp->get_stream( tSingleStreamNum );
        M3Stream* tFlCompStream = tWriteTest->GetStream( tFlCompStreamNum );
        M3ComplexDataWriter< f8_complex > tFlCompSData0( tFlCompStream->GetChannelRecord( 0 )->GetData(), 8, sAnalog, 2 );
        M3ComplexDataWriter< f8_complex > tFlCompSData1( tFlCompStream->GetChannelRecord( 1 )->GetData(), 8, sAnalog, 2 );
        M3ComplexDataWriter< f8_complex > tFlCompSData2( tFlCompStream->GetChannelRecord( 2 )->GetData(), 8, sAnalog, 2 );
        M3ComplexDataWriter< f8_complex > tFlCompSData3( tFlCompStream->GetChannelRecord( 3 )->GetData(), 8, sAnalog, 2 );
        M3ComplexDataWriter< f8_complex > tFlCompSData4( tFlCompStream->GetChannelRecord( 3 )->GetData(), 8, sAnalog, 2 );
        f8_complex value0, value1, value2, value3, value4;
        value0[ 0 ] = 0.0; value0[ 1 ] = 0.0;
        value1[ 0 ] = 1.1; value1[ 1 ] = 1.001;
        value2[ 0 ] = 2.2; value1[ 1 ] = 2.002;
        value3[ 0 ] = 3.3; value1[ 1 ] = 3.003;
        value4[ 0 ] = 4.4; value1[ 1 ] = 4.004;
        for( unsigned iSample = 0; iSample < tFlCompSamples; ++iSample )
        {
            tFlCompSData0.set_at( value0, iSample );
            tFlCompSData1.set_at( value1, iSample );
            tFlCompSData2.set_at( value2, iSample );
            tFlCompSData3.set_at( value3, iSample );
            tFlCompSData4.set_at( value4, iSample );
        }
        if( ! tFlCompStream->WriteRecord( true ) )
        {
            ERROR( plog, "Unable to write the record!" );
            delete tWriteTest;
            return -1;
        }

        value0[ 0 ] = -0.0; value0[ 1 ] = -0.0;
        value1[ 0 ] = -1.1; value1[ 1 ] = -1.001;
        value2[ 0 ] = -2.2; value1[ 1 ] = -2.002;
        value3[ 0 ] = -3.3; value1[ 1 ] = -3.003;
        value4[ 0 ] = -4.4; value1[ 1 ] = -4.004;
        for( unsigned iSample = 0; iSample < tFlCompSamples; ++iSample )
        {
            tFlCompSData0.set_at( value0, iSample );
            tFlCompSData1.set_at( value1, iSample );
            tFlCompSData2.set_at( value2, iSample );
            tFlCompSData3.set_at( value3, iSample );
            tFlCompSData4.set_at( value4, iSample );
        }
        if( ! tFlCompStream->WriteRecord( false ) )
        {
            ERROR( plog, "Unable to write the record!" );
            delete tWriteTest;
            return -1;
        }
        */


        //tWriteTest->FinishWriting();
        t_mwp->finish_file();
        INFO( plog, "File closed" );

        //delete tWriteTest;

    }
    catch( error& e )
    {
        ERROR( plog, "Exception thrown during write test:\n" << e.what() );
    }

    return 0;
}
