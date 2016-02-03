        /*
         * file:        homework.c
         * description: skeleton code for CS 5600 Homework 3
         *
         * Peter Desnoyers, Northeastern Computer Science, 2011
         * $Id: homework.c 410 2011-11-07 18:42:45Z pjd $
         */

        #include <stdio.h>
        #include <stdlib.h>
        #include <assert.h>
        #include "blkdev.h"
        #include <string.h>

        /********** MIRRORING ***************/

        /* example state for mirror device. See mirror_create for how to
         * initialize a struct blkdev with this.
         */
        struct mirror_dev {            
            struct blkdev *disks[2];    /* flag bad disk by setting to NULL */            
            int nblks;
        };
            
        // mirror_num_blocks: returns the number of blocks in the device dev   
        static int mirror_num_blocks(struct blkdev *dev)
        {
            struct mirror_dev *my_mirror_dev_obj;
            my_mirror_dev_obj = dev->private;
            return my_mirror_dev_obj->nblks;
        }

        /* read from one of the sides of the mirror. (if one side has failed,
         * it had better be the other one...) If both sides have failed,
         * return an error.
         * Note that a read operation may return an error to indicate that the
         * underlying device has failed, in which case you should close the
         * device and flag it (e.g. as a null pointer) so you won't try to use
         * it again. 
         */
        static int mirror_read(struct blkdev * dev, int first_blk,
                               int num_blks, void *buf)
        {
            /* your code here */
            struct mirror_dev *my_mirror_dev_obj = dev->private;
            //variable storing the disk 0 data
            struct blkdev *disk_1_data = my_mirror_dev_obj->disks[0];
            //variable storing the disk 1 data
            struct blkdev *disk_2_data = my_mirror_dev_obj->disks[1];
            
            int read_status = E_UNAVAIL;

            int flag1 = 0;
            int flag2 = 0;
            // if the disk 2 has not failed the we read the data into the buf
            if(disk_2_data != NULL){
             
                read_status = disk_2_data->ops->read(disk_2_data, first_blk, num_blks, buf);
             
                // checking if the status is bad address return error code.
                if(read_status == E_BADADDR){
             
                    return E_BADADDR;
                // checking if the status is unsvailable(failed) return error code.
                } else if(read_status == E_UNAVAIL){
             
                    // closing device and flagging it to NULL
                    disk_2_data->ops->close(disk_2_data);
             
                    my_mirror_dev_obj->disks[1] = NULL;

                } else {
                    // setting the flag incase this read is success and moving on to check the second disk
                    // just incase that disk is failed it will close that one and retiurn success.
                    flag1 += 1;
                
                }

            }

            // if the disk 1 has not failed the we read the data into the buf
            if(disk_1_data != NULL){
                
                // reading into buf
                read_status = disk_1_data->ops->read(disk_1_data, first_blk, num_blks, buf);
                
                // checking if the status is bad address return error code.
                if(read_status == E_BADADDR){
             
                    return E_BADADDR;
                // checking if the status is unsvailable(failed) return error code.
                } else if(read_status == E_UNAVAIL){
                    // closing device and flagging it to NULL
                    disk_1_data->ops->close(disk_1_data);
             
                    my_mirror_dev_obj->disks[0] = NULL;
                
                } else {
                    // setting the flag incase this read is success and moving on to check the second disk
                    // just incase that disk is failed it will close that one and retiurn success.
                    flag2 +=1;
                
                }

            }

            // if either of the read is success then return success else return the error code.
            if(flag1 > 0 || flag2 > 0)
                return SUCCESS;
            
            return read_status;
        }

        /* write to both sides of the mirror, or the remaining side if one has
         * failed. If both sides have failed, return an error.
         * Note that a write operation may indicate that the underlying device
         * has failed, in which case you should close the device and flag it
         * (e.g. as a null pointer) so you won't try to use it again.
         */
        static int mirror_write(struct blkdev * dev, int first_blk,
                                int num_blks, void *buf)
        {
            /* your code here */
            struct mirror_dev *my_mirror_dev_obj = dev->private;
            //variable storing the disk 0 data
            struct blkdev *disk_1_data = my_mirror_dev_obj->disks[0];
            //variable storing the disk 1 data
            struct blkdev *disk_2_data = my_mirror_dev_obj->disks[1];
            int badaddr_status_check1, badaddr_status_check2 = 0;
            int write_status_disk1, write_status_disk2 = E_UNAVAIL;
            // if the disk 1 has not failed the we read the data into the buf
            if(disk_1_data != NULL){
                //writing data into the disk 1 
                write_status_disk1 = disk_1_data->ops->write(disk_1_data, first_blk, num_blks, buf);
                
                // checking if the status is bad address return error code.
                if(write_status_disk1 == E_BADADDR){
             
                    badaddr_status_check1 = write_status_disk1;
                // checking if the status is unsvailable(failed) return error code.
                } else if(write_status_disk1 == E_UNAVAIL){
             
                    disk_1_data->ops->close(disk_1_data);
             
                    my_mirror_dev_obj->disks[0] = NULL;
                
                } 
            }

            if(disk_2_data != NULL){
             
                write_status_disk2 = disk_2_data->ops->write(disk_2_data, first_blk, num_blks, buf);
                 
                 // checking if the status is bad address return error code.
                if(write_status_disk2 == E_BADADDR){
                    badaddr_status_check2 = write_status_disk2;
                
                // checking if the status is unsvailable(failed) return error code.
                } else if(write_status_disk2 == E_UNAVAIL){
             
                    // closing device and flagging it to NULL
                    disk_2_data->ops->close(disk_2_data);
             
                    my_mirror_dev_obj->disks[1] = NULL;

                }

            }

             // if both the disks write functionality fails and returns bad address code then
             // return error.
            if((badaddr_status_check1 == E_BADADDR) 
                && (badaddr_status_check2 == E_BADADDR)){
                
                return E_BADADDR;
            
            } 
             // if both the disks write functionality fails and returns unsvailable (failed) code then
             // return error.
            if((write_status_disk1 == E_UNAVAIL) 
                && (write_status_disk2 == E_UNAVAIL)){
            
                return E_UNAVAIL;

            }
            
            return SUCCESS;
        }

        /* clean up, including: close any open (i.e. non-failed) devices, and
         * free any data structures you allocated in mirror_create.
         */
        static void mirror_close(struct blkdev *dev)
        {
            struct mirror_dev *my_mirror_dev_obj = dev->private;

            //variable storing the disk 0 data
            struct blkdev *disk_1_data = my_mirror_dev_obj->disks[0];
            
            //variable storing the disk 1 data
            struct blkdev *disk_2_data = my_mirror_dev_obj->disks[1];

            // if the disk 0 is not failed then close it and flag it to null
            if(disk_1_data != NULL){
            
                disk_1_data->ops->close(disk_1_data);
            
                my_mirror_dev_obj->disks[0] = NULL;
            
            }

            // if the disk 0 is not failed then close it and flag it to null
            if(disk_2_data != NULL){

                disk_2_data->ops->close(disk_2_data);
                
                my_mirror_dev_obj->disks[1] = NULL;        
            
            }

            // free the variable used inside the code.
            free(my_mirror_dev_obj);

            free(dev);
        }

        struct blkdev_ops mirror_ops = {
            .num_blocks = mirror_num_blocks,
            .read = mirror_read,
            .write = mirror_write,
            .close = mirror_close
        };

        /* create a mirrored volume from two disks. Do not write to the disks
         * in this function - you should assume that they contain identical
         * contents. 
         */
        struct blkdev *mirror_create(struct blkdev *disks[2])
        {
            struct blkdev *dev = malloc(sizeof(*dev));
            
            struct mirror_dev *mdev = malloc(sizeof(*mdev));

            struct blkdev *blockDev_1 = disks[0];
            
            struct blkdev *blockDev_2 = disks[1];

            // checking if the block size matches else return E_SIZE error.
            if(blockDev_1->ops->num_blocks(blockDev_1) 
                != blockDev_2->ops->num_blocks(blockDev_2)){
                
                printf("\n ERROR: Two given disks have different size \n");
                
                return NULL;

            }
            // setting the all the structure parametes of the mdev device.
            mdev->disks[1]=disks[1];
            mdev->disks[0]=disks[0];
            mdev->nblks = blockDev_1->ops->num_blocks(blockDev_1);
            dev->private = mdev;
            dev->ops = &mirror_ops;

            //return the newly created device.
            return dev;
        }

        /* replace failed device 'i' (0 or 1) in a mirror. Note that we assume
         * the upper layer knows which device failed. You will need to
         * replicate content from the other underlying device before returning
         * from this call.
         */
        int mirror_replace(struct blkdev *volume, int i, struct blkdev *newdisk)
        {
            struct mirror_dev *my_mirror_dev_obj = volume->private;

            //accessing the disk which is not being replaced.
            struct blkdev *disk_data = my_mirror_dev_obj->disks[1-i];
            //variable denotin the number of blocks in the volume to be replaced.
            int size_of_old_blk = my_mirror_dev_obj->nblks;
            //variable denotin the number of blocks in the new disk 
            int size_of_new_blk = newdisk->ops->num_blocks(newdisk);
            int array_size = BLOCK_SIZE * my_mirror_dev_obj->nblks;
            //declaring the temporary array in which we read the disk data.
            char temp_array[array_size];
            int read_status, write_status = E_UNAVAIL;
            // checking if the block size matches else return E_SIZE error.
            if(size_of_old_blk != size_of_new_blk){
            
                return E_SIZE;
            
            }
             
            // here we read the disk data into the temp_array
            read_status = disk_data->ops->read(disk_data, 0, my_mirror_dev_obj->nblks, temp_array);

            //checking the read success or not. else return failure if it fails.
            if(read_status != SUCCESS)
                return read_status;

            // here we write the disk data into the newdisk from the temp_array
            write_status = newdisk->ops->write(newdisk, 0, my_mirror_dev_obj->nblks, temp_array);

            //checking the write success or not. else return failure if it fails.
            if(write_status != SUCCESS)
                return read_status;

            //setting the disk whic needs the be replced 'i' with the newdisk which is the new one
            my_mirror_dev_obj->disks[i] = newdisk;

            return SUCCESS;

        }

        /**********  STRIPING ***************/

        struct my_strip_dev
        {
            int unit;               // number of units in each stripe set      
            int ndisks;             // number of disks in the entire drive
            int nblocks;            // number of blocks n the entire drive
            struct blkdev **disk;   // pointers to the disks
        };

        // stripe_num_blocks: returns the number of blocks in the device dev   
        int stripe_num_blocks(struct blkdev *dev)
        {
            struct my_strip_dev *stripe_dev = dev->private;            
            return stripe_dev->nblocks;
        }

        /* read blocks from a striped volume. 
         * Note that a read operation may return an error to indicate that the
         * underlying device has failed, in which case you should (a) close the
         * device and (b) return an error on this and all subsequent read or
         * write operations.
         */
        static int stripe_read(struct blkdev * dev, int first_blk,
                               int num_blks, void *buf)
        {
            struct my_strip_dev *stripe_dev = dev->private;
            //accessing the number of disks
            int number_of_disks = stripe_dev->ndisks;
            //accessing the number of units
            int number_of_units = stripe_dev->unit;
            int strip_number = 0;
            int strip_offset = 0;
            int disk_offset = 0;
            int disk_number = -1;
            int j = 0;
            int read_status = E_UNAVAIL;
            //toatl number of blocks is 'n', calculated by adding 
            //the first block number and the total number of blocks to read.
            int n = first_blk + num_blks;

            for(j=first_blk ; j< n ; j++){
                // strip number of the current block
                strip_number = j / number_of_units;
                // strip offset of the current block
                strip_offset = j % number_of_units;
                // offset in disk of the current block
                disk_offset  = (strip_number / number_of_disks)*number_of_units + strip_offset;

                if(strip_number < number_of_disks){
                    disk_number = strip_number;
                }
                else
                {
                    // disk number of the current block
                    disk_number = strip_number % number_of_disks;
                }
                
                //reading the current disk at the calculated disk_number into the buf
                read_status = stripe_dev->disk[disk_number]->ops->read(stripe_dev->disk[disk_number], disk_offset,1, buf);
                buf += BLOCK_SIZE;
                //verifying wheather the read was successful or not
                if(read_status == E_BADADDR){
                    return E_BADADDR;
                }
                else{
                    //verifying wheather the read was failed/unavailable
                    if(read_status == E_UNAVAIL){
                        //close the drive and return the error code
                        dev->ops->close(dev);
                        return E_UNAVAIL;
                    }
                }
            }
            return read_status;
        }

        /* write blocks to a striped volume.
         * Again if an underlying device fails you should close it and return
         * an error for this and all subsequent read or write operations.
         */
        static int stripe_write(struct blkdev * dev, int first_blk,
                                int num_blks, void *buf)
        {
            struct my_strip_dev *stripe_dev = dev->private;
            //accessing the number of disks
            int number_of_disks = stripe_dev->ndisks;
            //accessing the number of units
            int number_of_units = stripe_dev->unit;
            int strip_number = 0;
            int strip_offset = 0;
            int disk_offset = 0;
            int disk_number = -1;
            int j = 0;
            int read_status = E_UNAVAIL;
            //toatl number of blocks is 'n', calculated by adding 
            //the first block number and the total number of blocks to read.
            int n = first_blk + num_blks;

            for(j=first_blk ; j< n ; j++){
                // strip number of the current block
                strip_number = j / number_of_units;
                // strip offset of the current block
                strip_offset = j % number_of_units;
                // offset in disk of the current block
                disk_offset  = (strip_number / number_of_disks)*number_of_units + strip_offset;

                if(strip_number < number_of_disks){
                    disk_number = strip_number;
                }
                else
                {
                    // disk number of the current block
                    disk_number = strip_number % number_of_disks;
                }
                //writing into the current disk at the calculated disk_number from the buf
                read_status = stripe_dev->disk[disk_number]->ops->write(stripe_dev->disk[disk_number], disk_offset,1, buf);
                //incrementing the blobk size for the next iteration.
                buf += BLOCK_SIZE;
                //verifying wheather the read was successful or not
                if(read_status == E_BADADDR){
                    return E_BADADDR;
                //verifying wheather the read was failed/unavailable
                } else if(read_status == E_UNAVAIL){
                    //close the drive and return the error code (E_UNAVAIL)
                    dev->ops->close(dev);
                    return E_UNAVAIL;
                }
            }
            return read_status;
        }

        /* clean up, including: close all devices and free any data structures
         * you allocated in stripe_create. 
         */
        static void stripe_close(struct blkdev *dev)
        {
            struct my_strip_dev *stripe_dev = dev->private;
            int i =0;
            //creating the device structure which will access the disk of the strip drive
            struct blkdev **d = stripe_dev->disk;
            //iterating over the entire number of disks in the drive and closing each disk 
            //and flagging it to NULL
            for(i = 0; i< stripe_dev->ndisks ; i++){
                d[i]->ops->close(d[i]);
                d[i]=NULL;
            }
            //Relaeasing all the data structures used in the function.
            free(stripe_dev);
            free(dev);
        }

        struct blkdev_ops my_stripe_ops = 
        {
            .num_blocks = stripe_num_blocks,            
            .write = stripe_write,
            .close = stripe_close,
            .read = stripe_read
        };

        /* create a striped volume across N disks, with a stripe size of
         * 'unit'. (i.e. if 'unit' is 4, then blocks 0..3 will be on disks[0],
         * 4..7 on disks[1], etc.)
         * Check the size of the disks to compute the final volume size, and
         * fail (return NULL) if they aren't all the same.
         * Do not write to the disks in this function.
         */
        struct blkdev *striped_create(int N, struct blkdev *disks[], int unit)
        {
            //assigning the dev a memory allocation of itself
            struct blkdev *dev = malloc(sizeof(*dev));
            //assigning our strip device a memory allocation of size itself
            struct my_strip_dev *stripe_dev = malloc(sizeof(*stripe_dev));
            //accessing the number of blocks in the device.
            int my_disk_size = disks[0]->ops->num_blocks(disks[0]);
            int i =0;

            //comparing the number of blocks and the disk size(number of blocks in the disk)
            for(i = 1; i< N ; i++){
                //if they dont match then return NULL
                if(my_disk_size != disks[i]->ops->num_blocks(disks[i]))
                    return NULL;
            }
            //setting the rest of the parameters of the devie for creating it.
            stripe_dev->unit=unit;
            stripe_dev->nblocks = my_disk_size*N - ((my_disk_size*N) % unit);
            stripe_dev->disk = disks;
            stripe_dev->ndisks = N;
            dev->private = stripe_dev;
            dev->ops=&my_stripe_ops;
            return dev;

        }

        /**********   RAID 4  ***************/

       // structure defined for raid-4 device

        struct my_raid4_dev
        {
            int unit;               // number of sectors in a stripe            
            int ndisks;             // number of disks in the device            
            int nblocks;            // number of blocks in the device
            struct blkdev **disk;   // pointers to the disks
            int degraded_index_at;  // the disk number of the failed disk
            int is_degraded;        // variable representing whether a disk of the device has failed or not
            int nblocks_each_disk;  // number of blocks present in each disk
        };


        // function to return the number of blocks present in the device
        static int raid4_num_blocks(struct blkdev *dev)
        {
            struct my_raid4_dev *raid4_dev = dev->private;
            return raid4_dev->nblocks;

        }

        /* helper function - compute parity function across two blocks of
         * 'len' bytes and put it in a third block. Note that 'dst' can be the
         * same as either 'src1' or 'src2', so to compute parity across N
         * blocks you can do: 
         *
         *     void **block[i] - array of pointers to blocks
         *     dst = <zeros[len]>
         *     for (i = 0; i < N; i++)
         *        parity(block[i], dst, dst);
         *
         * Yes, it's slow.
         */
        void parity(int len, void *src1, void *src2, void *dst)
        {
            unsigned char *s1 = src1, *s2 = src2, *d = dst;
            int i;
            for (i = 0; i < len; i++)
                d[i] = s1[i] ^ s2[i];
        }

        // function to recover data in case of access to a failed disk

        static int recover_disk(struct blkdev * dev, int disk_offset, void *buf, int faulty_disk){

            struct my_raid4_dev *r4_dev = dev->private;
            // variable for number of disks in input device
            int number_of_disks = r4_dev->ndisks ;
            // array to read contents of the first disk read
            void *dst_array = (char *)malloc(BLOCK_SIZE);

            int read_status = E_UNAVAIL;
            int x = 0;
            // flag representing whether the disk to be read is the first one
            int flag = 1;

            // looping through all the disks present in the device 
            for(x = 0; x < number_of_disks; x++){
                // buffer of block size to store contents of various files
                void *source_array = (char *)malloc(BLOCK_SIZE);
                // checking whether the disk is null, implying failure 
                if(r4_dev->disk[x] != NULL){
                    // checking to see if disk has failed
                    if(x == r4_dev->degraded_index_at || x == faulty_disk)
                        // continue to other disks as the failed disk cannot be read 
                        continue;
                    // checking to see if the disk being read is the first one 
                    if(flag == 1){
                        // read one block of the disk into a buffer
                        read_status = r4_dev->disk[x]->ops->read(r4_dev->disk[x], disk_offset, 1, dst_array);
                        // checking if the read was successful
                        if(read_status != SUCCESS){
                            // freeing the buffer
                            free(source_array);
                            return read_status;
                        }
                        // setting flag to 0 as the first disk has already been read 
                        flag = 0;
                        continue;
                    }else{
                        // reading one block of the disk
                        read_status = r4_dev->disk[x]->ops->read(r4_dev->disk[x], disk_offset, 1, source_array);
                        // checking whether the read was successful
                        if(read_status != SUCCESS){
                            // freeing the buffer
                            free(source_array);
                            return read_status;
                        }
                        // calculating the parity between the buffer containing the data of the disk currently read with the 
                        // result of the xor operation on the disks read before the current one
                        parity(BLOCK_SIZE,dst_array,source_array,dst_array);
                        // freeing the buffer 
                        free(source_array);
                    }
                }
            }
            // copying the contents of the buffer containing result of the xoring operation into the input buffer location
            memcpy(buf,dst_array,BLOCK_SIZE);
            // freeing the buffer 
            free(dst_array);
            return read_status;
        }

        /* read blocks from a RAID 4 volume.
         * If the volume is in a degraded state you may need to reconstruct
         * data from the other stripes of the stripe set plus parity.
         * If a drive fails during a read and all other drives are
         * operational, close that drive and continue in degraded state.
         * If a drive fails and the volume is already in a degraded state,
         * close the drive and return an error.
         */

        static int raid4_read(struct blkdev * dev, int first_blk,
                              int num_blks, void *buf){

            struct my_raid4_dev *r4_dev = dev->private;
            // variable for the number of data disks (excluding parity)
            int number_of_disks = r4_dev->ndisks - 1;
            // number of blocks in a stripe 
            int number_of_units = r4_dev->unit;

            // variables
            int strip_number = 0;
            int strip_offset = 0;
            int disk_offset = 0;
            int disk_number = -1;
            int j = 0;
            int read_status = E_UNAVAIL;
            // the number of blocks that need to be read 
            int n = first_blk + num_blks;

            // looping through the blocks to be read starting from the given start block 
            for(j=first_blk ; j< n ; j++){
                // calculating the stripe number of the block
                strip_number = j / number_of_units;
                // offset of the stripe the block is contained in
                strip_offset = j % number_of_units;
                // offset of the block in the disk 
                disk_offset  = (strip_number / number_of_disks)*number_of_units + strip_offset;
                // calculating the disk the block is contained in 
                if(strip_number < number_of_disks){
                    disk_number = strip_number;
                } 
                else{
                    disk_number = strip_number % number_of_disks;
                }

                // checking whether the disk is null, implying has already failed 

                if(r4_dev->disk[disk_number] != NULL){

                    // reading a block from the disk 
                    read_status = r4_dev->disk[disk_number]->ops->read(r4_dev->disk[disk_number], disk_offset,1, buf);
                    // checking if the error obtained in result is that of bad address
                    if(read_status == E_BADADDR){            
                        return E_BADADDR;             
                    } 
                    // checking if the error obtained in the result is device unavailable
                    else if(read_status == E_UNAVAIL){
                        // checking to see if the disk is already failed and the device containing the disk has a failed disk
                        if(r4_dev->is_degraded == 1 && r4_dev->degraded_index_at != disk_number) {
                            // closing the device due to multiple disk failures 
                            dev->ops->close(dev);
                            // returning the device unavailable error 
                            return E_UNAVAIL;
                        }

                        // setting the disk failed index to the current disk
                        r4_dev->degraded_index_at = disk_number;
                        // setting the variable containing whether the device's disk has failed to 1 as we have encountered a failure 
                        r4_dev->is_degraded = 1;             
                        // closing the current failed disk 
                        r4_dev->disk[disk_number]->ops->close(r4_dev->disk[disk_number]); 
                        // setting the failed disk to null
                        r4_dev->disk[disk_number] = NULL; 
                        // recovering the data of the failed disk as there is only one failed disk in the device                
                        read_status = recover_disk(dev, disk_offset, buf, disk_number);
                        if(read_status != SUCCESS)
                           return read_status;
                    }
                } else{


                    // checking whether the device has a failed disk and the failed disk is not the current one,
                    // indicating multiple disk failures 
                    if(r4_dev->is_degraded == 1 && r4_dev->degraded_index_at != disk_number) {
                        // closing the device 
                        dev->ops->close(dev);
                        return E_UNAVAIL;
                    }

                        // setting the failed index to the current disk
                        r4_dev->degraded_index_at = disk_number;
                        // setting that the device has a failed disk 
                        r4_dev->is_degraded = 1;                     
                        // recovering data of the failed disk as there is only one failure 
                        read_status = recover_disk(dev, disk_offset, buf,disk_number);
                        if(read_status != SUCCESS)
                            return read_status;                                                                                                                                                                                   
                }
                // incrementing the buffer by block size to continue reading the next block
                buf += BLOCK_SIZE;
            }

        return read_status;
    }

        /* write blocks to a RAID 4 volume.
         * Note that you must handle short writes - i.e. less than a full
         * stripe set. You may either use the optimized algorithm (for N>3
         * read old data, parity, write new data, new parity) or you can read
         * the entire stripe set, modify it, and re-write it. Your code will
         * be graded on correctness, not speed.
         * If an underlying device fails you should close it and complete the
         * write in the degraded state. If a drive fails in the degraded
         * state, close it and return an error.
         * In the degraded state perform all writes to non-failed drives, and
         * forget about the failed one. (parity will handle it)
         */
        static int raid4_write(struct blkdev * dev, int first_blk,
                               int num_blks, void *buf)
        {
            struct my_raid4_dev *r4_dev = dev->private;
            // number of disks in the device
            int number_of_disks = r4_dev->ndisks - 1;
            // number of blocks in a stripe of the given device 
            int number_of_units = r4_dev->unit;
            // variables 
            int strip_number = 0;
            int strip_offset = 0;
            int disk_offset = 0;
            int disk_number = -1;
            int j = 0;
            int read_status = E_UNAVAIL;

            // total number of blocks to be written 
            int n = first_blk + num_blks;
            // buffers 
            void *old_array = (char *)malloc(BLOCK_SIZE);
            void *parity_array = (char *)malloc(BLOCK_SIZE);
            int write_status = E_UNAVAIL;
            
            // looping through the blocks to be written starting with the given first block 
            for(j=first_blk ; j< n ; j++){
                // calculating stripe number of the block 
                strip_number = j / number_of_units;       
                // calculating stripe offset of the block in the disk         
                strip_offset = j % number_of_units;                  
                // calculating the offset of the block in the disk   
                disk_offset  = (strip_number / number_of_disks)*number_of_units + strip_offset;                    
                // finding the disk number of the current block 
                if(strip_number < number_of_disks){
                    disk_number = strip_number;
                } 
                else{
                    disk_number = strip_number % number_of_disks;
                }
                // checking whether the current disk is null 
                if(r4_dev->disk[disk_number] != NULL){
                    // reading a block from the current disk
                    read_status = r4_dev->disk[disk_number]->ops->read(r4_dev->disk[disk_number], disk_offset,1, old_array);
                    
                    // checking to see if the error returned is that of a bad address
                    if(read_status == E_BADADDR){
                        return E_BADADDR;
                    } 

                    else
                    {
                        // checking to see if the error returned is that of device unavailable 
                        if(read_status == E_UNAVAIL){
                            // checking to see if the device has a failed disk which is different from the current one 
                            if(r4_dev->is_degraded == 1 && r4_dev->degraded_index_at != disk_number) {
                                // closing the device due to multiple failures
                                dev->ops->close(dev);
                                return E_UNAVAIL;
                            }
                        
                            // setting the failed disk number of the device to the current disk
                            r4_dev->degraded_index_at = disk_number;
                            // setting that the device has a failed disk
                            r4_dev->is_degraded = 1; 
                            // closing the failed disk
                            r4_dev->disk[disk_number]->ops->close(r4_dev->disk[disk_number]); 
                            // setting the failed current disk to null 
                            r4_dev->disk[disk_number] = NULL;
                            // recovering the data of the failed disk as the device has only a single failed disk
                            read_status = recover_disk(dev, disk_offset, old_array, disk_number);
                            if(read_status != SUCCESS){
                                return read_status;
                            }

                        }
 
                    }
                }

                else{
                    // checking if the device has a failed disk which is different from the current one 
                    if(r4_dev->is_degraded == 1 && r4_dev->degraded_index_at != disk_number) {
                        // closing the device due to multiple disk failures 
                        dev->ops->close(dev);
                        return E_UNAVAIL;
                    }
                        // setting the current disk as the failed disk in the device 
                        r4_dev->degraded_index_at = disk_number;
                        // setting that the device contains a failed disk 
                        r4_dev->is_degraded = 1; 
                        // recovering the data of the failed disk as the device has only a single failed disk
                        read_status = recover_disk(dev, disk_offset, old_array, disk_number);
                        if(read_status != SUCCESS){
                            return read_status;
                        }
                }

                // reading a block from the parity disk 
                read_status = r4_dev->disk[number_of_disks]->ops->read(r4_dev->disk[number_of_disks], disk_offset,1,parity_array);
                // checking to see if the error returned is that of a bad address 
                if(read_status == E_BADADDR){
                    return E_BADADDR;
                } 

                else
                { 
                    // checking to see if the error returned is that of disk unavailable 
                    if(read_status == E_UNAVAIL){
                        // checking to see if the drive has a failed disk which is different from the current one 
                        if(r4_dev->is_degraded == 1 && r4_dev->degraded_index_at != number_of_disks) {
                            // closing the device due to multiple failures 
                            dev->ops->close(dev);
                            return E_UNAVAIL;
                        }
                    // setting the parity disk as the number of the failed disk of the drive
                    r4_dev->degraded_index_at = number_of_disks;
                    // setting that the device contains a failed parrity disk 
                    r4_dev->is_degraded = 1;
                    // closing the failed parity drive  
                    r4_dev->disk[number_of_disks]->ops->close(r4_dev->disk[number_of_disks]); 
                    // setting the parity disk to null as it has failed 
                    r4_dev->disk[disk_number] = NULL;
                    // recovering the data present in the failed parity disk as it is the only failed disk in the drive 
                    read_status = recover_disk(dev, disk_offset, parity_array, disk_number);

                    if(read_status != SUCCESS){
                        return read_status;
                    }
                    }
                }

                // xoring the parity block with the block of data read from the current disk 
                parity(BLOCK_SIZE, old_array, parity_array, parity_array);

                // checking to see that the current disk has not failed so that we can write into it
                if(r4_dev->degraded_index_at != disk_number){
                    // writing the buffer to the current disk 
                    write_status = r4_dev->disk[disk_number]->ops->write(r4_dev->disk[disk_number], disk_offset,1, buf);
                    if(write_status != SUCCESS){
                        return write_status;
                    }
                }

                // xoring the buffer with the parity buffer
                parity(BLOCK_SIZE, buf, parity_array, parity_array);    

                // checking whether the parity disk has failed 
                if(r4_dev->degraded_index_at != number_of_disks){
                    // writing the parity to the disk as it has not failed 
                    write_status = r4_dev->disk[number_of_disks]->ops->write(r4_dev->disk[number_of_disks], disk_offset,1,parity_array);
                }

                // incrementing the buffer to start writing the next block 
                buf += BLOCK_SIZE;
            }

            // freeing the buffers 
            free(old_array);
            free(parity_array);         
            return read_status;
        }

        /* clean up, including: close all devices and free any data structures
         * you allocated in raid4_create. 
         */
        static void raid4_close(struct blkdev *dev)
        {
            struct my_raid4_dev *r4_dev = dev->private;
            int i =0;
            //creating the array of pointers to the disk
            struct blkdev **d = r4_dev->disk;
            for(i = 0; i< r4_dev->ndisks ; i++){
                //if the disk number is not the degraded one then we 
                //close that disk and set it to NULL
                if (i != r4_dev->degraded_index_at){
                    d[i]->ops->close(d[i]);
                    d[i]=NULL;
                }
            
            }
            //freeing the data structurea used
            free(r4_dev);
            free(dev);
        }

        struct blkdev_ops my_raid4_ops = 
        {
            .num_blocks = raid4_num_blocks,
            .write = raid4_write,
            .close = raid4_close,
            .read = raid4_read
        };


        /* Initialize a RAID 4 volume with stripe size 'unit', using
         * disks[N-1] as the parity drive. Do not write to the disks - assume
         * that they are properly initialized with correct parity. (warning -
         * some of the grading scripts may fail if you modify data on the
         * drives in this function)
         */
        struct blkdev *raid4_create(int N, struct blkdev *disks[], int unit)
        {
            //assigning the dev a memory allocation of itself
            struct blkdev *dev = malloc(sizeof(*dev));
            //assigning our strip device a memory allocation of size itself
            struct my_raid4_dev *r4_dev = malloc(sizeof(*r4_dev));
            //accessing the number of blocks in the device.
            int my_disk_size = disks[0]->ops->num_blocks(disks[0]);

            int i =0;

            for(i = 1; i< N ; i++){
                //verifying if the sizes match, else throw error.                
                if(my_disk_size != disks[i]->ops->num_blocks(disks[i]))
                    return NULL;
            }
            //setting rest of the parameters of the device structure
            r4_dev->ndisks = N;        
            r4_dev->unit=unit;
            r4_dev->nblocks = (my_disk_size * (N - 1)) - ((my_disk_size * (N - 1)) % unit);
            r4_dev->disk = disks;
            r4_dev->nblocks_each_disk = my_disk_size;
            // initially set to -1 since no degaraded index present
            r4_dev->degraded_index_at = -1; 
            r4_dev->is_degraded = 0;        
            dev->private = r4_dev;
            dev->ops = &my_raid4_ops;
            //calling replace to replace the parity disk.
            //this thing here is done in order to recaculate the parity and
            // /save the actual parity in the parity disk
            raid4_replace(dev,N-1, disks[N-1]);
            return dev;
        }

        /* replace failed device 'i' in a RAID 4. Note that we assume
         * the upper layer knows which device failed. You will need to
         * reconstruct content from data and parity before returning
         * from this call.
         */

        int raid4_replace(struct blkdev *volume, int i, struct blkdev *newdisk)
        {

            struct my_raid4_dev *my_raid4_obj = volume->private;
            // variable representing the number of blocks present on each disk
            int size_of_old_blk = my_raid4_obj->nblocks_each_disk;
            // the number of blocks present in the new disk 
            int size_of_new_blk = newdisk->ops->num_blocks(newdisk);
            // the number of disks in the drive
            int number_of_disks = my_raid4_obj->ndisks;
            // reducing the number of disks by 1 as the disk count starts from 0
            number_of_disks = number_of_disks - 1;
            // number of blocks in a stripe
            int number_of_units = my_raid4_obj->unit;
            // variable representing the status of write to a disk
            int write_status = E_UNAVAIL;

            // checking whether the number of blocks in the disk to be replaced is same
            // as the one replacing it
            if(size_of_old_blk != size_of_new_blk){
                // returning the size error in case of size mismatch
                return E_SIZE;
            }
           

            int block = 0;
            int recover_status = E_UNAVAIL;
            // number of blocks that can actually be filled with data as the number of blocks in the disk 
            // may not be a multiple of the stripe size leading to blocks that will be left empty
            int num_blocks_limit = size_of_old_blk - (size_of_old_blk % number_of_units);

            // looping through the blocks to be filled with data of the disk the new one is replacing 
            for(block = 0; block < num_blocks_limit; block++ ){
                // buffer
                void *buf = (char *)malloc(BLOCK_SIZE);
                // recover the data of the disk to be replaced in case of failure
                recover_status = recover_disk(volume, block, buf,i);
                // checking for successful recovery of the data 
                if(recover_status != SUCCESS){
                    // freeing the buffer
                    free(buf);
                    return recover_status;
                }

                // writing the block to the new disk after its data has been successfully recovered 
                // from the failure of the old disk
                write_status = newdisk->ops->write(newdisk, block, 1, buf);
                // checking for successful write
                if(write_status != SUCCESS){
                    // freeing buffer
                    free(buf);
                    return write_status;
                }
                // freeing the buffer 
                free(buf);
            }

            // checking if the disk to be replaced was the only failed disk in the drive
            if(my_raid4_obj->degraded_index_at == i){
                // setting the variable to represent that the drive has all operational disks
                my_raid4_obj->degraded_index_at = -1;
                my_raid4_obj->is_degraded = 0;
            }

            // replacing the old disk by the new one in the drive
            my_raid4_obj->disk[i] = newdisk;

            return SUCCESS;
             

        }
