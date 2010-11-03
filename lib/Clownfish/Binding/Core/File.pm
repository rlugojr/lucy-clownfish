# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

use strict;
use warnings;

package Clownfish::Binding::Core::File;
use Clownfish::Util qw( a_isa_b verify_args );
use Clownfish::Binding::Core::Class;
use File::Spec::Functions qw( catfile splitpath );
use File::Path qw( mkpath );
use Scalar::Util qw( blessed );
use Fcntl;
use Carp;

my %write_h_PARAMS = (
    file   => undef,
    dest   => undef,
    header => undef,
    footer => undef,
);

sub write_h {
    my ( undef, %args ) = @_;
    verify_args( \%write_h_PARAMS, %args ) or confess $@;
    my $file = $args{file};
    confess("Not a Clownfish::File")
        unless a_isa_b( $file, "Clownfish::File" );
    my $h_path = $file->h_path( $args{dest} );

    # Unlink then open file.
    my ( undef, $out_dir, undef ) = splitpath($h_path);
    mkpath $out_dir unless -d $out_dir;
    confess("Can't make dir '$out_dir'") unless -d $out_dir;
    unlink $h_path;
    sysopen( my $fh, $h_path, O_CREAT | O_EXCL | O_WRONLY )
        or confess("Can't open '$h_path' for writing");

    # Create the include-guard strings.
    my $include_guard_start = $file->guard_start;
    my $include_guard_close = $file->guard_close;

    # Aggregate block content.
    my $content = "";
    for my $block ( $file->blocks ) {
        if ( a_isa_b( $block, 'Clownfish::Parcel' ) ) { }
        elsif ( a_isa_b( $block, 'Clownfish::Class' ) ) {
            my $class_binding
                = Clownfish::Binding::Core::Class->new( client => $block, );
            $content .= $class_binding->to_c_header . "\n";
        }
        elsif ( a_isa_b( $block, 'Clownfish::CBlock' ) ) {
            $content .= $block->get_contents . "\n";
        }
        else {
            confess("Invalid block: $block");
        }
    }

    print $fh <<END_STUFF;
$args{header}

$include_guard_start

#ifdef __cplusplus
extern "C" {
#endif

$content

#ifdef __cplusplus
}
#endif

$include_guard_close

$args{footer}

END_STUFF
}

my %write_c_PARAMS = (
    file   => undef,
    dest   => undef,
    header => undef,
    footer => undef,
);

sub write_c {
    my ( undef, %args ) = @_;
    verify_args( \%write_h_PARAMS, %args ) or confess $@;
    my $file = $args{file};
    confess("Not a Clownfish::File")
        unless a_isa_b( $file, "Clownfish::File" );
    my $c_path = $file->c_path( $args{dest} );

    # Unlink then open file.
    my ( undef, $out_dir, undef ) = splitpath($c_path);
    mkpath $out_dir unless -d $out_dir;
    confess("Can't make dir '$out_dir'") unless -d $out_dir;
    unlink $c_path;
    sysopen( my $fh, $c_path, O_CREAT | O_EXCL | O_WRONLY )
        or confess("Can't open '$c_path' for writing");

    # Aggregate content.
    my $content     = "";
    my $c_file_syms = "";
    for my $block ( $file->blocks ) {
        if ( blessed($block) ) {
            if ( $block->isa('Clownfish::Class') ) {
                my $bound
                    = Clownfish::Binding::Core::Class->new( client => $block,
                    );
                $content .= $bound->to_c . "\n";
                my $c_file_sym = "C_" . uc( $block->full_struct_sym );

                # Temporary hack.
                $c_file_sym =~ s/KINO/LUCY/;

                $c_file_syms .= "#define $c_file_sym\n";
            }
        }
    }

    print $fh <<END_STUFF;
$args{header}

$c_file_syms
#define C_LUCY_VTABLE
#define C_LUCY_ZOMBIECHARBUF
#include "boil.h"
#include "KinoSearch/Object/VTable.h"
#include "KinoSearch/Object/CharBuf.h"
#include "KinoSearch/Object/Err.h"
#include "KinoSearch/Object/Hash.h"
#include "KinoSearch/Object/Host.h"
#include "KinoSearch/Object/VArray.h"

$content

$args{footer}

END_STUFF
}

1;

__END__

__POD__

=head1 NAME

Clownfish::Binding::Core::File - Generate core C code for a Clownfish file.

=head1 DESCRIPTION

This module is the companion to Clownfish::File, generating the C code
needed to implement the file's specification.

There is a one-to-one mapping between Clownfish header files and autogenerated
.h and .c files.  If Foo.cfh includes both Foo and Foo::FooJr, then it is
necessary to pound-include "Foo.h" in order to get FooJr's interface -- not
"Foo/FooJr.h", which won't exist.

=head1 CLASS METHODS

=head2 write_h

    Clownfish::Binding::Core::File->write_c(
        file   => $file,                            # required
        dest   => '/path/to/autogen_dir',           # required
        header => "/* Autogenerated file. */\n",    # required
        footer => $copyfoot,                        # required
    );

Generate a C header file containing all class declarations and literal C
blocks.

=over

=item * B<file> - A L<Clownfish::File>.

=item * B<dest> - The directory under which autogenerated files are being
written.

=item * B<header> - Text which will be prepended to each generated C file --
typically, an "autogenerated file" warning.

=item * B<footer> - Text to be appended to the end of each generated C file --
typically copyright information.

=back 

=head2 write_h

    Clownfish::Binding::Core::File->write_c(
        file   => $file,                            # required
        dest   => '/path/to/autogen_dir',           # required
        header => "/* Autogenerated file. */\n",    # required
        footer => $copyfoot,                        # required
    );

Generate a C file containing code needed by the class implementations.

=cut
