/* Author: Ralf Willenbacher, June 2022 */
/* License: Consider it Public Domain, but some countries do not allow to drop copyright */
/*
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <memory.h>



int main( int i_argc, char *rgpc_argv[] )
{
	FILE *pf_in, *pf_out;
	char rgc_out_fname[ 1024 ];
#define BUFFER_SIZE 0x100000
	uint8_t *pui8_buffer;
	int32_t i_buffer_occupancy, i_offset, i_found_offset, i_next_found_offset, i_found_count, i_slot, i_payload_end, i_forced_match;
	int64_t i64_stream_offset;
	size_t ui_ret;

	if( i_argc < 2 )
	{
		fprintf( stderr, "usage: h265xrepacker <file>\n" );
		exit( 0 );
	}

	pf_in = fopen( rgpc_argv[ 1 ], "rb" );
	if( pf_in )
	{
		sprintf( rgc_out_fname, "%s.264", rgpc_argv[ 1 ] );
		pf_out = fopen( rgc_out_fname, "wb" );
		if( !pf_out )
		{
			fprintf( stderr, "error: unable to open output file\n" );
			exit( 1 );
		}
	}
	else
	{
		fprintf( stderr, "error: unable to open input file\n" );
		exit( 1 );
	}

	i_buffer_occupancy = 0;
	pui8_buffer = malloc( BUFFER_SIZE * sizeof( uint8_t ) );
	if( !pui8_buffer )
	{
		fprintf( stderr, "error: unable to allocate buffer\n" );
		exit( 1 );
	}

	i64_stream_offset = 0;
	i_found_count = 0;
	i_next_found_offset = i_found_offset = 0;
	i_forced_match = 0;

	while( 1 )
	{
		ui_ret = fread( pui8_buffer + i_buffer_occupancy, sizeof( uint8_t ), BUFFER_SIZE - i_buffer_occupancy, pf_in );
		if( ui_ret > 0 )
		{
			i_buffer_occupancy += ( int32_t )ui_ret;
		}

		if( i_forced_match > 0 )
		{
			i_offset = i_forced_match;
		}
		else
		{
			i_offset = i_next_found_offset;
		}
		i_next_found_offset = -1;

		while( i_offset < i_buffer_occupancy - 3 )
		{
			if( pui8_buffer[ i_offset ] == 0 && pui8_buffer[ i_offset + 1 ] == 0 && pui8_buffer[ i_offset + 2 ] == 1 )
			{
				i_next_found_offset = i_offset;
				i_offset = 3;
				break;
			}
			i_offset++;
		}

		if( i_forced_match > 0 && ( ( i_next_found_offset != -1 ) && ( i_next_found_offset != i_forced_match ) ) )
		{
			fprintf( stderr, "error: forced match failed\n" );
			exit( 1 );
		}
		else
		{
			fprintf( stderr, "forced match matched\n" );
		}


		if( i_next_found_offset >= 0 )
		{
			i_payload_end = i_next_found_offset;
		}
		else
		{
			i_payload_end = i_buffer_occupancy;
		}




		if( i_found_count > 0 )
		{
			if( i_slot == 0 )
			{
				fprintf( stderr, "  discarding.\n" );
			}
			else if( i_slot == 1 )
			{
				uint8_t ui8_trail = 0x80;
				fprintf( stderr, "  saving.\n" );
				fwrite( pui8_buffer + i_found_offset, sizeof( uint8_t ), i_payload_end - i_found_offset, pf_out );
				//fwrite( &ui8_trail, sizeof( uint8_t ), 1, pf_out );
			}
			i_found_count--;
		}

		memmove( pui8_buffer, pui8_buffer + i_payload_end, i_buffer_occupancy - i_payload_end );
		i_buffer_occupancy -= i_payload_end;
		i64_stream_offset += i_payload_end;
		if( i_next_found_offset > 0 )
		{
			i_next_found_offset = 0;
		}


		if( i_buffer_occupancy == 0 )
		{
			fprintf( stderr, "end of data at %08lld\n", i64_stream_offset );
			break;
		}

		if( i_next_found_offset >= 0 )
		{
			int32_t i_type;

			i_found_offset = i_next_found_offset;
			i_next_found_offset = 2;
			i_found_count++;

			i_type = pui8_buffer[ i_offset ];
			i_forced_match = 0;
			if( i_type & 0x80 )
			{
				fprintf( stderr, "%08lld: unknown type 0x%02x\n", i64_stream_offset, i_type );
				if( i_type == 0xfa )
				{
					i_forced_match = i_offset + 5 + ( pui8_buffer[ i_offset + 3 ] | ( pui8_buffer[ i_offset + 4 ] << 8 ) );
				}
				i_slot = 0;
			}
			else
			{
				int32_t i_nal_unit_type = i_type & 0x1f;
				int32_t i_nal_unit_ref = ( i_type & 0x60 ) >> 5;
				fprintf( stderr, "%08lld: nalu type 0x%02x ( ref %d )\n", i64_stream_offset, i_nal_unit_type, i_nal_unit_ref );
				if( i_nal_unit_type == 6 )
				{
					i_slot = 0;
				}
				else
				{
					i_slot = 1;
				}
			}
		}
	}



	fclose( pf_in );
	fclose( pf_out );
	free( pui8_buffer );

	return 0;
}











